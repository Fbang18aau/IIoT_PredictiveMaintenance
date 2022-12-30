///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_AWS_MQTT.c
///
/// @note Project    : IIoT_Client/Gateway
/// @note Subsystem  : AWS_MQTT
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "D_AWS_MQTT.h"


// === Definition of macro / constants ============================================================

/**
 * @brief Packet Identifier generated when Subscribe request was sent to the broker;
 * it is used to match received Subscribe ACK to the transmitted subscribe.
 */
static uint16_t globalSubscribePacketIdentifier = 0U;

/**
 * @brief Packet Identifier generated when Unsubscribe request was sent to the broker;
 * it is used to match received Unsubscribe ACK to the transmitted unsubscribe
 * request.
 */
static uint16_t globalUnsubscribePacketIdentifier = 0U;

/**
 * @brief Array to keep the outgoing publish messages.
 * These stored outgoing publish messages are kept until a successful ack
 * is received.
 */
static PublishPackets_t outgoingPublishPackets[ MAX_OUTGOING_PUBLISHES ] = { 0 };

/**
 * @brief Array to keep subscription topics.
 * Used to re-subscribe to topics that failed initial subscription attempts.
 */
static MQTTSubscribeInfo_t pGlobalSubscriptionList[ 1 ];

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context.
 */
static uint8_t buffer[ NETWORK_BUFFER_SIZE ];

/**
 * @brief Status of latest Subscribe ACK;
 * it is updated every time the callback function processes a Subscribe ACK
 * and accounts for subscription to a single topic.
 */
static MQTTSubAckStatus_t globalSubAckStatus = MQTTSubAckFailure;

/**
 * @brief Static buffer for TLS Context Semaphore.
 */
static StaticSemaphore_t xTlsContextSemaphoreBuffer;

// === Definition of global variables =============================================================
MQTTContext_t     mqttContext = { 0 };
NetworkContext_t  xNetworkContext = { 0 };
bool mqttSessionEstablished = false;

// === Definition of local variables ==============================================================


// === Class/function implementation ==============================================================



/*-----------------------------------------------------------*/

static uint32_t generateRandomNumber()
{
    return( rand() );
}

/*-----------------------------------------------------------*/
int connectToServerWithBackoffRetries( NetworkContext_t * pNetworkContext )
{
    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    TlsTransportStatus_t tlsStatus = TLS_TRANSPORT_SUCCESS;
    BackoffAlgorithmContext_t reconnectParams;

    pNetworkContext->pcHostname = AWS_IOT_ENDPOINT;
    pNetworkContext->xPort = AWS_MQTT_PORT;
    pNetworkContext->pxTls = NULL;
    pNetworkContext->xTlsContextSemaphore = xSemaphoreCreateMutexStatic(&xTlsContextSemaphoreBuffer);

    pNetworkContext->disableSni = 0;
    uint16_t nextRetryBackOff;

    /* Initialize credentials for establishing TLS session. */
    pNetworkContext->pcServerRootCAPem = root_cert_auth_pem_start;

    /* If #CLIENT_USERNAME is defined, username/password is used for authenticating
     * the client. */
#ifdef CONFIG_EXAMPLE_USE_SECURE_ELEMENT
    pNetworkContext->pcClientCertPem = NULL;
    pNetworkContext->pcClientKeyPem = NULL;
    pNetworkContext->use_secure_element = true;
#elif CONFIG_EXAMPLE_USE_DS_PERIPHERAL
    pNetworkContext->pcClientCertPem = client_cert_pem_start;
    pNetworkContext->pcClientKeyPem = NULL;
#error "Populate the ds_data structure and remove this line"
    /* pNetworkContext->ds_data = DS_DATA; */
    /* The ds_data can be populated using the API's provided by esp_secure_cert_mgr */
#else
    #ifndef CLIENT_USERNAME
        pNetworkContext->pcClientCertPem = client_cert_pem_start;
        pNetworkContext->pcClientKeyPem = client_key_pem_start;
    #endif
#endif
    /* AWS IoT requires devices to send the Server Name Indication (SNI)
     * extension to the Transport Layer Security (TLS) protocol and provide
     * the complete endpoint address in the host_name field. Details about
     * SNI for AWS IoT can be found in the link below.
     * https://docs.aws.amazon.com/iot/latest/developerguide/transport-security.html */

    if( AWS_MQTT_PORT == 443 )
    {
        /* Pass the ALPN protocol name depending on the port being used.
         * Please see more details about the ALPN protocol for the AWS IoT MQTT
         * endpoint in the link below.
         * https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/
         *
         * For username and password based authentication in AWS IoT,
         * #AWS_IOT_PASSWORD_ALPN is used. More details can be found in the
         * link below.
         * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
         */

        static const char * pcAlpnProtocols[] = { NULL, NULL };

        #ifdef CLIENT_USERNAME
            pcAlpnProtocols[0] = AWS_IOT_PASSWORD_ALPN;
        #else
            pcAlpnProtocols[0] = AWS_IOT_MQTT_ALPN;
        #endif

        pNetworkContext->pAlpnProtos = pcAlpnProtocols;
    } else {
        pNetworkContext->pAlpnProtos = NULL;
    }

    /* Initialize reconnect attempts and interval */
    BackoffAlgorithm_InitializeParams( &reconnectParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase until maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects
         * to the MQTT broker as specified in AWS_IOT_ENDPOINT and AWS_MQTT_PORT
         * at the demo config header. */
        LogInfo( ( "Establishing a TLS session to %.*s:%d.",
                   AWS_IOT_ENDPOINT_LENGTH,
                   AWS_IOT_ENDPOINT,
                   AWS_MQTT_PORT ) );
        tlsStatus = xTlsConnect ( pNetworkContext );

        if( tlsStatus != TLS_TRANSPORT_SUCCESS )
        {
            /* Generate a random number and get back-off value (in milliseconds) for the next connection retry. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &reconnectParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the broker failed, all attempts exhausted." ) );
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the broker failed. Retrying connection "
                           "after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff ) );
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( tlsStatus != TLS_TRANSPORT_SUCCESS ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int getNextFreeIndexForOutgoingPublishes( uint8_t * pIndex )
{
    int returnStatus = EXIT_FAILURE;
    uint8_t index = 0;

    assert( outgoingPublishPackets != NULL );
    assert( pIndex != NULL );

    for( index = 0; index < MAX_OUTGOING_PUBLISHES; index++ )
    {
        /* A free index is marked by invalid packet id.
         * Check if the the index has a free slot. */
        if( outgoingPublishPackets[ index ].packetId == MQTT_PACKET_ID_INVALID )
        {
            returnStatus = EXIT_SUCCESS;
            break;
        }
    }

    /* Copy the available index into the output param. */
    *pIndex = index;

    return returnStatus;
}
/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishAt( uint8_t index )
{
    assert( outgoingPublishPackets != NULL );
    assert( index < MAX_OUTGOING_PUBLISHES );

    /* Clear the outgoing publish packet. */
    ( void ) memset( &( outgoingPublishPackets[ index ] ),
                     0x00,
                     sizeof( outgoingPublishPackets[ index ] ) );
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishes( void )
{
    assert( outgoingPublishPackets != NULL );

    /* Clean up all the outgoing publish packets. */
    ( void ) memset( outgoingPublishPackets, 0x00, sizeof( outgoingPublishPackets ) );
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishWithPacketID( uint16_t packetId )
{
    uint8_t index = 0;

    assert( outgoingPublishPackets != NULL );
    assert( packetId != MQTT_PACKET_ID_INVALID );

    /* Clean up all the saved outgoing publishes. */
    for( ; index < MAX_OUTGOING_PUBLISHES; index++ )
    {
        if( outgoingPublishPackets[ index ].packetId == packetId )
        {
            cleanupOutgoingPublishAt( index );
            LogInfo( ( "Cleaned up outgoing publish packet with packet id %u.\n\n",
                       packetId ) );
            break;
        }
    }
}

/*-----------------------------------------------------------*/

static int handlePublishResend( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t index = 0U;
    MQTTStateCursor_t cursor = MQTT_STATE_CURSOR_INITIALIZER;
    uint16_t packetIdToResend = MQTT_PACKET_ID_INVALID;
    bool foundPacketId = false;

    assert( pMqttContext != NULL );
    assert( outgoingPublishPackets != NULL );

    /* MQTT_PublishToResend() provides a packet ID of the next PUBLISH packet
     * that should be resent. In accordance with the MQTT v3.1.1 spec,
     * MQTT_PublishToResend() preserves the ordering of when the original
     * PUBLISH packets were sent. The outgoingPublishPackets array is searched
     * through for the associated packet ID. If the application requires
     * increased efficiency in the look up of the packet ID, then a hashmap of
     * packetId key and PublishPacket_t values may be used instead. */
    packetIdToResend = MQTT_PublishToResend( pMqttContext, &cursor );

    while( packetIdToResend != MQTT_PACKET_ID_INVALID )
    {
        foundPacketId = false;

        for( index = 0U; index < MAX_OUTGOING_PUBLISHES; index++ )
        {
            if( outgoingPublishPackets[ index ].packetId == packetIdToResend )
            {
                foundPacketId = true;
                outgoingPublishPackets[ index ].pubInfo.dup = true;

                LogInfo( ( "Sending duplicate PUBLISH with packet id %u.",
                           outgoingPublishPackets[ index ].packetId ) );
                mqttStatus = MQTT_Publish( pMqttContext,
                                           &outgoingPublishPackets[ index ].pubInfo,
                                           outgoingPublishPackets[ index ].packetId );

                if( mqttStatus != MQTTSuccess )
                {
                    LogError( ( "Sending duplicate PUBLISH for packet id %u "
                                " failed with status %s.",
                                outgoingPublishPackets[ index ].packetId,
                                MQTT_Status_strerror( mqttStatus ) ) );
                    returnStatus = EXIT_FAILURE;
                    break;
                }
                else
                {
                    LogInfo( ( "Sent duplicate PUBLISH successfully for packet id %u.\n\n",
                               outgoingPublishPackets[ index ].packetId ) );
                }
            }
        }

        if( foundPacketId == false )
        {
            LogError( ( "Packet id %u requires resend, but was not found in "
                        "outgoingPublishPackets.",
                        packetIdToResend ) );
            returnStatus = EXIT_FAILURE;
            break;
        }
        else
        {
            /* Get the next packetID to be resent. */
            packetIdToResend = MQTT_PublishToResend( pMqttContext, &cursor );
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void handleIncomingPublish( MQTTPublishInfo_t * pPublishInfo,
                                   uint16_t packetIdentifier )
{
    assert( pPublishInfo != NULL );

    /* Process incoming Publish. */
    LogInfo( ( "Incoming QOS : %d.", pPublishInfo->qos ) );

    /* Verify the received publish is for the topic we have subscribed to. */
    if( ( pPublishInfo->topicNameLength == MQTT_EXAMPLE_TOPIC_LENGTH ) &&
        ( 0 == strncmp( MQTT_EXAMPLE_TOPIC,
                        pPublishInfo->pTopicName,
                        pPublishInfo->topicNameLength ) ) )
    {
        LogInfo( ( "Incoming Publish Topic Name: %.*s matches subscribed topic.\n"
                   "Incoming Publish message Packet Id is %u.\n"
                   "Incoming Publish Message : %.*s.\n\n",
                   pPublishInfo->topicNameLength,
                   pPublishInfo->pTopicName,
                   packetIdentifier,
                   ( int ) pPublishInfo->payloadLength,
                   ( const char * ) pPublishInfo->pPayload ) );
    }
    else
    {
        LogInfo( ( "Incoming Publish Topic Name: %.*s does not match subscribed topic.",
                   pPublishInfo->topicNameLength,
                   pPublishInfo->pTopicName ) );
    }
}

/*-----------------------------------------------------------*/

static void updateSubAckStatus( MQTTPacketInfo_t * pPacketInfo )
{
    uint8_t * pPayload = NULL;
    size_t pSize = 0;

    MQTTStatus_t mqttStatus = MQTT_GetSubAckStatusCodes( pPacketInfo, &pPayload, &pSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    assert( mqttStatus == MQTTSuccess );

    /* Suppress unused variable warning when asserts are disabled in build. */
    ( void ) mqttStatus;

    /* Demo only subscribes to one topic, so only one status code is returned. */
    globalSubAckStatus = pPayload[ 0 ];
}

/*-----------------------------------------------------------*/

static int handleResubscribe( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryParams;
    uint16_t nextRetryBackOff = 0U;

    assert( pMqttContext != NULL );

    /* Initialize retry attempts and interval. */
    BackoffAlgorithm_InitializeParams( &retryParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    do
    {
        /* Send SUBSCRIBE packet.
         * Note: reusing the value specified in globalSubscribePacketIdentifier is acceptable here
         * because this function is entered only after the receipt of a SUBACK, at which point
         * its associated packet id is free to use. */
        mqttStatus = MQTT_Subscribe( pMqttContext,
                                     pGlobalSubscriptionList,
                                     sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                     globalSubscribePacketIdentifier );

        if( mqttStatus != MQTTSuccess )
        {
            LogError( ( "Failed to send SUBSCRIBE packet to broker with error = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
            returnStatus = EXIT_FAILURE;
            break;
        }

        LogInfo( ( "SUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_EXAMPLE_TOPIC_LENGTH,
                   MQTT_EXAMPLE_TOPIC ) );

        /* Process incoming packet. */
        mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

        if( mqttStatus != MQTTSuccess )
        {
            LogError( ( "MQTT_ProcessLoop returned with status = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
            returnStatus = EXIT_FAILURE;
            break;
        }

        /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
         * in eventCallback to reflect the status of the SUBACK sent by the broker. It represents
         * either the QoS level granted by the server upon subscription, or acknowledgement of
         * server rejection of the subscription request. */
        if( globalSubAckStatus == MQTTSubAckFailure )
        {
            /* Generate a random number and get back-off value (in milliseconds) for the next re-subscribe attempt. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &retryParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Subscription to topic failed, all attempts exhausted." ) );
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Server rejected subscription request. Retrying "
                           "connection after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff ) );
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( globalSubAckStatus == MQTTSubAckFailure ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void eventCallback( MQTTContext_t * pMqttContext,
                           MQTTPacketInfo_t * pPacketInfo,
                           MQTTDeserializedInfo_t * pDeserializedInfo )
{
    uint16_t packetIdentifier;

    assert( pMqttContext != NULL );
    assert( pPacketInfo != NULL );
    assert( pDeserializedInfo != NULL );

    /* Suppress unused parameter warning when asserts are disabled in build. */
    ( void ) pMqttContext;

    packetIdentifier = pDeserializedInfo->packetIdentifier;

    /* Handle incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if( ( pPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( pDeserializedInfo->pPublishInfo != NULL );
        /* Handle incoming publish. */
        handleIncomingPublish( pDeserializedInfo->pPublishInfo, packetIdentifier );
    }
    else
    {
        /* Handle other packets. */
        switch( pPacketInfo->type )
        {
            case MQTT_PACKET_TYPE_SUBACK:

                /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
                 * It contains the status code indicating server approval/rejection for the subscription to the single topic
                 * requested. The SUBACK will be parsed to obtain the status code, and this status code will be stored in global
                 * variable globalSubAckStatus. */
                updateSubAckStatus( pPacketInfo );

                /* Check status of the subscription request. If globalSubAckStatus does not indicate
                 * server refusal of the request (MQTTSubAckFailure), it contains the QoS level granted
                 * by the server, indicating a successful subscription attempt. */
                if( globalSubAckStatus != MQTTSubAckFailure )
                {
                    LogInfo( ( "Subscribed to the topic %.*s. with maximum QoS %u.\n\n",
                               MQTT_EXAMPLE_TOPIC_LENGTH,
                               MQTT_EXAMPLE_TOPIC,
                               globalSubAckStatus ) );
                }

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalSubscribePacketIdentifier == packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
                LogInfo( ( "Unsubscribed from the topic %.*s.\n\n",
                           MQTT_EXAMPLE_TOPIC_LENGTH,
                           MQTT_EXAMPLE_TOPIC ) );
                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalUnsubscribePacketIdentifier == packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_PINGRESP:

                /* Nothing to be done from application as library handles
                 * PINGRESP. */
                LogWarn( ( "PINGRESP should not be handled by the application "
                           "callback when using MQTT_ProcessLoop.\n\n" ) );
                break;

            case MQTT_PACKET_TYPE_PUBACK:
                LogInfo( ( "PUBACK received for packet id %u.\n\n",
                           packetIdentifier ) );
                /* Cleanup publish packet when a PUBACK is received. */
                cleanupOutgoingPublishWithPacketID( packetIdentifier );
                break;

            /* Any other packet type is invalid. */
            default:
                LogError( ( "Unknown packet type received:(%02x).\n\n",
                            pPacketInfo->type ) );
        }
    }
}

/*-----------------------------------------------------------*/

static int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo = { 0 };

    assert( pMqttContext != NULL );
    assert( pSessionPresent != NULL );

    /* Establish MQTT session by sending a CONNECT packet. */

    /* If #createCleanSession is true, start with a clean session
     * i.e. direct the MQTT broker to discard any previous session data.
     * If #createCleanSession is false, directs the broker to attempt to
     * reestablish a session which was already present. */
    connectInfo.cleanSession = createCleanSession;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    connectInfo.pClientIdentifier = CLIENT_IDENTIFIER;
    connectInfo.clientIdentifierLength = CLIENT_IDENTIFIER_LENGTH;

    /* The maximum time interval in seconds which is allowed to elapse
     * between two Control Packets.
     * It is the responsibility of the Client to ensure that the interval between
     * Control Packets being sent does not exceed the this Keep Alive value. In the
     * absence of sending any other Control Packets, the Client MUST send a
     * PINGREQ Packet. */
    connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    /* Use the username and password for authentication, if they are defined.
     * Refer to the AWS IoT documentation below for details regarding client
     * authentication with a username and password.
     * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
     * An authorizer setup needs to be done, as mentioned in the above link, to use
     * username/password based client authentication.
     *
     * The username field is populated with voluntary metrics to AWS IoT.
     * The metrics collected by AWS IoT are the operating system, the operating
     * system's version, the hardware platform, and the MQTT Client library
     * information. These metrics help AWS IoT improve security and provide
     * better technical support.
     *
     * If client authentication is based on username/password in AWS IoT,
     * the metrics string is appended to the username to support both client
     * authentication and metrics collection. */
    #ifdef CLIENT_USERNAME
        connectInfo.pUserName = CLIENT_USERNAME_WITH_METRICS;
        connectInfo.userNameLength = strlen( CLIENT_USERNAME_WITH_METRICS );
        connectInfo.pPassword = CLIENT_PASSWORD;
        connectInfo.passwordLength = strlen( CLIENT_PASSWORD );
    #else
        connectInfo.pUserName = METRICS_STRING;
        connectInfo.userNameLength = METRICS_STRING_LENGTH;
        /* Password for authentication is not used. */
        connectInfo.pPassword = NULL;
        connectInfo.passwordLength = 0U;
    #endif /* ifdef CLIENT_USERNAME */

    /* Send MQTT CONNECT packet to broker. */
    mqttStatus = MQTT_Connect( pMqttContext, &connectInfo, NULL, CONNACK_RECV_TIMEOUT_MS, pSessionPresent );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        LogError( ( "Connection with MQTT broker failed with status %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
    }
    else
    {
        LogInfo( ( "MQTT connection successfully established with broker.\n\n" ) );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int disconnectMqttSession( MQTTContext_t * pMqttContext )
{
    MQTTStatus_t mqttStatus = MQTTSuccess;
    int returnStatus = EXIT_SUCCESS;

    assert( pMqttContext != NULL );

    /* Send DISCONNECT. */
    mqttStatus = MQTT_Disconnect( pMqttContext );

    if( mqttStatus != MQTTSuccess )
    {
        LogError( ( "Sending MQTT DISCONNECT failed with status=%s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int subscribeToTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    /* This example subscribes to only one topic and uses QOS1. */
    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS1;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = MQTT_EXAMPLE_TOPIC;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = MQTT_EXAMPLE_TOPIC_LENGTH;

    /* Generate packet identifier for the SUBSCRIBE packet. */
    globalSubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send SUBSCRIBE packet. */
    mqttStatus = MQTT_Subscribe( pMqttContext,
                                 pGlobalSubscriptionList,
                                 sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                 globalSubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
        LogError( ( "Failed to send SUBSCRIBE packet to broker with error = %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
        returnStatus = EXIT_FAILURE;
    }
    else
    {
        LogInfo( ( "SUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_EXAMPLE_TOPIC_LENGTH,
                   MQTT_EXAMPLE_TOPIC ) );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int unsubscribeFromTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    /* This example subscribes to and unsubscribes from only one topic
     * and uses QOS1. */
    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS1;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = MQTT_EXAMPLE_TOPIC;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = MQTT_EXAMPLE_TOPIC_LENGTH;

    /* Generate packet identifier for the UNSUBSCRIBE packet. */
    globalUnsubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send UNSUBSCRIBE packet. */
    mqttStatus = MQTT_Unsubscribe( pMqttContext,
                                   pGlobalSubscriptionList,
                                   sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                   globalUnsubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
        LogError( ( "Failed to send UNSUBSCRIBE packet to broker with error = %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
        returnStatus = EXIT_FAILURE;
    }
    else
    {
        LogInfo( ( "UNSUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_EXAMPLE_TOPIC_LENGTH,
                   MQTT_EXAMPLE_TOPIC ) );
    }

    return returnStatus;
}

// void D_AWS_MQTT_CreateTestPayload(uint16_t * pPayload)
// {
//     char cPayloadHeader[] = "{\"Vibration_X\":";

//     char cPayloadYHeader[] = "\"Vibration_Y\":";
//     char cPayloadZHeader[] = "\"Vibration_Z\":";

//     char cPayloadTemp[10];

//     char cPayloadDataFooter2[] = ",";

//     char cPayloadFooter[] = "}";


//     strcpy(pPayload, cPayloadHeader);

//     itoa(tBMX160_Accel[0].s16X_Axis, cPayloadTemp, 10);
//     strcat(pPayload, cPayloadTemp);
//     strcat(pPayload, cPayloadDataFooter2);


//     strcat(pPayload, cPayloadYHeader);

//     itoa(tBMX160_Accel[0].s16Y_Axis, cPayloadTemp, 10);
//     strcat(pPayload, cPayloadTemp);
//     strcat(pPayload, cPayloadDataFooter2);


//     strcat(pPayload, cPayloadZHeader);

//     itoa(tBMX160_Accel[0].s16Z_Axis, cPayloadTemp, 10);
//     strcat(pPayload, cPayloadTemp);

//     strcat(pPayload, cPayloadFooter);
// }


void D_AWS_MQTT_CreatePayload(uint16_t * pPayload)
{
    char cPayloadHeader[] = "{\"Time\":";

    // char cPayloadXHeader[] = "\"Vibration_X\":{";
    // char cPayloadYHeader[] = "\"Vibration_Y\":{";
    // char cPayloadZHeader[] = "\"Vibration_Z\":{";

    char cPayloadTemp[40];

    char cPayloadIdexHeaderX[] = "X";
    char cPayloadIdexHeaderY[] = "Y";
    char cPayloadIdexHeaderZ[] = "Z";

    char cPayloadDataHeader[] = "\"";

    char cPayloadDataFooter1[] = "\":";
    char cPayloadDataFooter2[] = ",";

    char cPayloadFooter1[] = "}";

    time_t timeNow;
    struct tm  ts;

    // Time
    strcpy(pPayload, cPayloadHeader);

    time(&timeNow);

    itoa(timeNow, cPayloadTemp, 10);

    // strcat(pPayload, cPayloadDataHeader);
    strcat(pPayload, cPayloadTemp);

    // strcat(pPayload, cPayloadDataHeader);
    strcat(pPayload, cPayloadDataFooter2);

    // X axis

    // strcat(pPayload, cPayloadXHeader);

    for(uint16_t u16X = 0; u16X < D_AWS_MQTT_VIBRATION_SIZE; u16X++)
    {
        strcat(pPayload, cPayloadDataHeader);
        strcat(pPayload, cPayloadIdexHeaderX);

        itoa(u16X, cPayloadTemp, 10);
        strcat(pPayload, cPayloadTemp);
        strcat(pPayload, cPayloadDataFooter1);

        itoa(tBMX160_Accel[u16X].s16X_Axis, cPayloadTemp, 10);

        strcat(pPayload, cPayloadTemp);

        if(u16X < D_AWS_MQTT_VIBRATION_SIZE - 1)
        {
            strcat(pPayload, cPayloadDataFooter2);
        }

    }
    // strcat(pPayload, cPayloadFooter1);
    strcat(pPayload, cPayloadDataFooter2);

    // Y axis

    // strcat(pPayload, cPayloadYHeader);
    for(uint16_t u16X = 0; u16X < D_AWS_MQTT_VIBRATION_SIZE; u16X++)
    {
        strcat(pPayload, cPayloadDataHeader);
        strcat(pPayload, cPayloadIdexHeaderY);

        itoa(u16X, cPayloadTemp, 10);
        strcat(pPayload, cPayloadTemp);
        strcat(pPayload, cPayloadDataFooter1);

        itoa(tBMX160_Accel[u16X].s16Y_Axis, cPayloadTemp, 10);

        strcat(pPayload, cPayloadTemp);

        if(u16X < D_AWS_MQTT_VIBRATION_SIZE-1)
        {
            strcat(pPayload, cPayloadDataFooter2);
        }

    }

    // strcat(pPayload, cPayloadFooter1);
    strcat(pPayload, cPayloadDataFooter2);

    // Z axis

    // strcat(pPayload, cPayloadZHeader);
    for(uint16_t u16X = 0; u16X < D_AWS_MQTT_VIBRATION_SIZE; u16X++)
    {
        strcat(pPayload, cPayloadDataHeader);
        strcat(pPayload, cPayloadIdexHeaderZ);

        itoa(u16X, cPayloadTemp, 10);
        strcat(pPayload, cPayloadTemp);
        strcat(pPayload, cPayloadDataFooter1);

        itoa(tBMX160_Accel[u16X].s16Z_Axis, cPayloadTemp, 10);

        strcat(pPayload, cPayloadTemp);

        if(u16X < D_AWS_MQTT_VIBRATION_SIZE-1)
        {
            strcat(pPayload, cPayloadDataFooter2);
        }

    }

    // strcat(pPayload, cPayloadFooter1);

    strcat(pPayload, cPayloadFooter1);
}

/*-----------------------------------------------------------*/

static int publishToTopic( MQTTContext_t * pMqttContext, uint16_t * pTransmitData)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t publishIndex = MAX_OUTGOING_PUBLISHES;

    assert( pMqttContext != NULL );

    /* Get the next free index for the outgoing publish. All QoS1 outgoing
     * publishes are stored until a PUBACK is received. These messages are
     * stored for supporting a resend if a network connection is broken before
     * receiving a PUBACK. */
    returnStatus = getNextFreeIndexForOutgoingPublishes( &publishIndex );

    if( returnStatus == EXIT_FAILURE )
    {
        LogError( ( "Unable to find a free spot for outgoing PUBLISH message.\n\n" ) );
    }
    else
    {
        /* This example publishes to only one topic and uses QOS1. */
        outgoingPublishPackets[ publishIndex ].pubInfo.qos = MQTTQoS1;
        outgoingPublishPackets[ publishIndex ].pubInfo.pTopicName = MQTT_EXAMPLE_TOPIC;
        outgoingPublishPackets[ publishIndex ].pubInfo.topicNameLength = MQTT_EXAMPLE_TOPIC_LENGTH;
        // outgoingPublishPackets[ publishIndex ].pubInfo.pPayload = MQTT_EXAMPLE_MESSAGE;
        // outgoingPublishPackets[ publishIndex ].pubInfo.payloadLength = MQTT_EXAMPLE_MESSAGE_LENGTH;

        // outgoingPublishPackets[ publishIndex ].pubInfo.pPayload = pTransmitData;
        // outgoingPublishPackets[ publishIndex ].pubInfo.payloadLength = ((sizeof(pTransmitData) * 2) - 2);

        char cPayload[D_AWS_MQTT_PAYLOAD_SIZE];

        if(xSemaphoreTake(xVibrationSemaphore, MQTT_WAIT_TIME / portTICK_PERIOD_MS) == pdTRUE)
        {
            D_AWS_MQTT_CreatePayload(cPayload);
            // D_AWS_MQTT_CreateTestPayload(cPayload);

            // Clear Sensor dataset
            for(uint16_t u16X = 0; u16X < VIBRATION_DATA_LENGTH; u16X++)
            {
                tBMX160_Accel[u16X].s16X_Axis = 0;
                tBMX160_Accel[u16X].s16Y_Axis = 0;
                tBMX160_Accel[u16X].s16Z_Axis = 0;
            }

            // Give semaphore
            xSemaphoreGive(xVibrationSemaphore);
        }
        else
        {
            ESP_LOGE(D_AWS_MQTT_TAG, "Create Payload FAILED");
        }

         outgoingPublishPackets[ publishIndex ].pubInfo.pPayload = cPayload;
         outgoingPublishPackets[ publishIndex ].pubInfo.payloadLength = (strlen(cPayload));

        /* Get a new packet id. */
        outgoingPublishPackets[ publishIndex ].packetId = MQTT_GetPacketId( pMqttContext );

        /* Send PUBLISH packet. */
        mqttStatus = MQTT_Publish( pMqttContext,
                                   &outgoingPublishPackets[ publishIndex ].pubInfo,
                                   outgoingPublishPackets[ publishIndex ].packetId );

        if( mqttStatus != MQTTSuccess )
        {
            LogError( ( "Failed to send PUBLISH packet to broker with error = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
            cleanupOutgoingPublishAt( publishIndex );
            returnStatus = EXIT_FAILURE;
        }
        else
        {
            LogInfo( ( "PUBLISH sent for topic %.*s to broker with packet ID %u.\n\n",
                       MQTT_EXAMPLE_TOPIC_LENGTH,
                       MQTT_EXAMPLE_TOPIC,
                       outgoingPublishPackets[ publishIndex ].packetId ) );
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int initializeMqtt( MQTTContext_t * pMqttContext,
                           NetworkContext_t * pNetworkContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t networkBuffer;
    TransportInterface_t transport;

    assert( pMqttContext != NULL );
    assert( pNetworkContext != NULL );

    /* Fill in TransportInterface send and receive function pointers.
     * For this demo, TCP sockets are used to send and receive data
     * from network. Network context is SSL context for OpenSSL.*/
    transport.pNetworkContext = pNetworkContext;
    transport.send = espTlsTransportSend;
    transport.recv = espTlsTransportRecv;

    /* Fill the values for network buffer. */
    networkBuffer.pBuffer = buffer;
    networkBuffer.size = NETWORK_BUFFER_SIZE;

    /* Initialize MQTT library. */
    mqttStatus = MQTT_Init( pMqttContext,
                            &transport,
                            Clock_GetTimeMs,
                            eventCallback,
                            &networkBuffer );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        LogError( ( "MQTT init failed: Status = %s.", MQTT_Status_strerror( mqttStatus ) ) );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

// int subscribePublishLoop( MQTTContext_t * pMqttContext,
//                                  bool * pClientSessionPresent )
// {
//     int returnStatus = EXIT_SUCCESS;
//     bool mqttSessionEstablished = false, brokerSessionPresent;
//     MQTTStatus_t mqttStatus = MQTTSuccess;
//     uint32_t publishCount = 0;
//     const uint32_t maxPublishCount = MQTT_PUBLISH_COUNT_PER_LOOP;
//     bool createCleanSession = false;

//     assert( pMqttContext != NULL );
//     assert( pClientSessionPresent != NULL );

//     /* A clean MQTT session needs to be created, if there is no session saved
//      * in this MQTT client. */
//     createCleanSession = ( *pClientSessionPresent == true ) ? false : true;

//     /* Establish MQTT session on top of TCP+TLS connection. */
//     LogInfo( ( "Creating an MQTT connection to %.*s.",
//                AWS_IOT_ENDPOINT_LENGTH,
//                AWS_IOT_ENDPOINT ) );

//     /* Sends an MQTT Connect packet using the established TLS session,
//      * then waits for connection acknowledgment (CONNACK) packet. */
//     returnStatus = establishMqttSession( pMqttContext, createCleanSession, &brokerSessionPresent );


//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* Keep a flag for indicating if MQTT session is established. This
//          * flag will mark that an MQTT DISCONNECT has to be sent at the end
//          * of the demo, even if there are intermediate failures. */
//         mqttSessionEstablished = true;

//         /* Update the flag to indicate that an MQTT client session is saved.
//          * Once this flag is set, MQTT connect in the following iterations of
//          * this demo will be attempted without requesting for a clean session. */
//         *pClientSessionPresent = true;

//         /* Check if session is present and if there are any outgoing publishes
//          * that need to resend. This is only valid if the broker is
//          * re-establishing a session which was already present. */
//         if( brokerSessionPresent == true )
//         {
//             LogInfo( ( "An MQTT session with broker is re-established. "
//                        "Resending unacked publishes." ) );

//             /* Handle all the resend of publish messages. */
//             returnStatus = handlePublishResend( pMqttContext );
//         }
//         else
//         {
//             LogInfo( ( "A clean MQTT connection is established."
//                        " Cleaning up all the stored outgoing publishes.\n\n" ) );

//             /* Clean up the outgoing publishes waiting for ack as this new
//              * connection doesn't re-establish an existing session. */
//             cleanupOutgoingPublishes();
//         }
//     }

//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* The client is now connected to the broker. Subscribe to the topic
//          * as specified in MQTT_EXAMPLE_TOPIC at the top of this file by sending a
//          * subscribe packet. This client will then publish to the same topic it
//          * subscribed to, so it will expect all the messages it sends to the broker
//          * to be sent back to it from the broker. This demo uses QOS1 in Subscribe,
//          * therefore, the Publish messages received from the broker will have QOS1. */
//         LogInfo( ( "Subscribing to the MQTT topic %.*s.",
//                    MQTT_EXAMPLE_TOPIC_LENGTH,
//                    MQTT_EXAMPLE_TOPIC ) );
//         returnStatus = subscribeToTopic( pMqttContext );
//     }

//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* Process incoming packet from the broker. Acknowledgment for subscription
//          * ( SUBACK ) will be received here. However after sending the subscribe, the
//          * client may receive a publish before it receives a subscribe ack. Since this
//          * demo is subscribing to the topic to which no one is publishing, probability
//          * of receiving publish message before subscribe ack is zero; but application
//          * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
//          * receive packet from network. */
//         mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

//         if( mqttStatus != MQTTSuccess )
//         {
//             returnStatus = EXIT_FAILURE;
//             LogError( ( "MQTT_ProcessLoop returned with status = %s.",
//                         MQTT_Status_strerror( mqttStatus ) ) );
//         }
//     }

//     /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
//      * in eventCallback to reflect the status of the SUBACK sent by the broker. */
//     if( ( returnStatus == EXIT_SUCCESS ) && ( globalSubAckStatus == MQTTSubAckFailure ) )
//     {
//         /* If server rejected the subscription request, attempt to resubscribe to topic.
//          * Attempts are made according to the exponential backoff retry strategy
//          * implemented in retryUtils. */
//         LogInfo( ( "Server rejected initial subscription request. Attempting to re-subscribe to topic %.*s.",
//                    MQTT_EXAMPLE_TOPIC_LENGTH,
//                    MQTT_EXAMPLE_TOPIC ) );
//         returnStatus = handleResubscribe( pMqttContext );
//     }

//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* Publish messages with QOS1, receive incoming messages and
//          * send keep alive messages. */
//         for( publishCount = 0; publishCount < maxPublishCount; publishCount++ )
//         {
//             LogInfo( ( "Sending Publish to the MQTT topic %.*s.",
//                        MQTT_EXAMPLE_TOPIC_LENGTH,
//                        MQTT_EXAMPLE_TOPIC ) );
//             returnStatus = publishToTopic( pMqttContext );

//             /* Calling MQTT_ProcessLoop to process incoming publish echo, since
//              * application subscribed to the same topic the broker will send
//              * publish message back to the application. This function also
//              * sends ping request to broker if MQTT_KEEP_ALIVE_INTERVAL_SECONDS
//              * has expired since the last MQTT packet sent and receive
//              * ping responses. */
//             mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

//             /* For any error in #MQTT_ProcessLoop, exit the loop and disconnect
//              * from the broker. */
//             if( mqttStatus != MQTTSuccess )
//             {
//                 LogError( ( "MQTT_ProcessLoop returned with status = %s.",
//                             MQTT_Status_strerror( mqttStatus ) ) );
//                 returnStatus = EXIT_FAILURE;
//                 break;
//             }

//             LogInfo( ( "Delay before continuing to next iteration.\n\n" ) );

//             /* Leave connection idle for some time. */
//             sleep( DELAY_BETWEEN_PUBLISHES_SECONDS );
//         }
//     }

//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* Unsubscribe from the topic. */
//         LogInfo( ( "Unsubscribing from the MQTT topic %.*s.",
//                    MQTT_EXAMPLE_TOPIC_LENGTH,
//                    MQTT_EXAMPLE_TOPIC ) );
//         returnStatus = unsubscribeFromTopic( pMqttContext );
//     }

//     if( returnStatus == EXIT_SUCCESS )
//     {
//         /* Process Incoming UNSUBACK packet from the broker. */
//         mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

//         if( mqttStatus != MQTTSuccess )
//         {
//             returnStatus = EXIT_FAILURE;
//             LogError( ( "MQTT_ProcessLoop returned with status = %s.",
//                         MQTT_Status_strerror( mqttStatus ) ) );
//         }
//     }

//     /* Send an MQTT Disconnect packet over the already connected TCP socket.
//      * There is no corresponding response for the disconnect packet. After sending
//      * disconnect, client must close the network connection. */
//     if( mqttSessionEstablished == true )
//     {
//         LogInfo( ( "Disconnecting the MQTT connection with %.*s.",
//                    AWS_IOT_ENDPOINT_LENGTH,
//                    AWS_IOT_ENDPOINT ) );

//         if( returnStatus == EXIT_FAILURE )
//         {
//             /* Returned status is not used to update the local status as there
//              * were failures in demo execution. */
//             ( void ) disconnectMqttSession( pMqttContext );
//         }
//         else
//         {
//             returnStatus = disconnectMqttSession( pMqttContext );
//         }
//     }

//     /* Reset global SUBACK status variable after completion of subscription request cycle. */
//     globalSubAckStatus = MQTTSubAckFailure;

//     return returnStatus;
// }


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief 
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
int EstablishAndSubscribe(MQTTContext_t * pMqttContext, bool * pClientSessionPresent)
{
  int returnStatus = EXIT_SUCCESS;
  bool brokerSessionPresent;
  MQTTStatus_t mqttStatus = MQTTSuccess;
  // uint32_t publishCount = 0;
  // const uint32_t maxPublishCount = MQTT_PUBLISH_COUNT_PER_LOOP;
  bool createCleanSession = false;

  assert( pMqttContext != NULL );
  assert( pClientSessionPresent != NULL );

  /* A clean MQTT session needs to be created, if there is no session saved
    * in this MQTT client. */
  createCleanSession = ( *pClientSessionPresent == true ) ? false : true;

  /* Establish MQTT session on top of TCP+TLS connection. */
  LogInfo( ( "Creating an MQTT connection to %.*s.",
              AWS_IOT_ENDPOINT_LENGTH,
              AWS_IOT_ENDPOINT ) );

  /* Sends an MQTT Connect packet using the established TLS session,
    * then waits for connection acknowledgment (CONNACK) packet. */
  returnStatus = establishMqttSession( pMqttContext, createCleanSession, &brokerSessionPresent );


  if( returnStatus == EXIT_SUCCESS )
  {
      /* Keep a flag for indicating if MQTT session is established. This
        * flag will mark that an MQTT DISCONNECT has to be sent at the end
        * of the demo, even if there are intermediate failures. */
      mqttSessionEstablished = true;

      /* Update the flag to indicate that an MQTT client session is saved.
        * Once this flag is set, MQTT connect in the following iterations of
        * this demo will be attempted without requesting for a clean session. */
      *pClientSessionPresent = true;

      /* Check if session is present and if there are any outgoing publishes
        * that need to resend. This is only valid if the broker is
        * re-establishing a session which was already present. */
      if( brokerSessionPresent == true )
      {
          LogInfo( ( "An MQTT session with broker is re-established. "
                      "Resending unacked publishes." ) );

          /* Handle all the resend of publish messages. */
          returnStatus = handlePublishResend( pMqttContext );
      }
      else
      {
          LogInfo( ( "A clean MQTT connection is established."
                      " Cleaning up all the stored outgoing publishes.\n\n" ) );

          /* Clean up the outgoing publishes waiting for ack as this new
            * connection doesn't re-establish an existing session. */
          cleanupOutgoingPublishes();
      }
  }

  if( returnStatus == EXIT_SUCCESS )
  {
      /* The client is now connected to the broker. Subscribe to the topic
        * as specified in MQTT_EXAMPLE_TOPIC at the top of this file by sending a
        * subscribe packet. This client will then publish to the same topic it
        * subscribed to, so it will expect all the messages it sends to the broker
        * to be sent back to it from the broker. This demo uses QOS1 in Subscribe,
        * therefore, the Publish messages received from the broker will have QOS1. */
      LogInfo( ( "Subscribing to the MQTT topic %.*s.",
                  MQTT_EXAMPLE_TOPIC_LENGTH,
                  MQTT_EXAMPLE_TOPIC ) );
      returnStatus = subscribeToTopic( pMqttContext );
  }

  if( returnStatus == EXIT_SUCCESS )
  {
      /* Process incoming packet from the broker. Acknowledgment for subscription
        * ( SUBACK ) will be received here. However after sending the subscribe, the
        * client may receive a publish before it receives a subscribe ack. Since this
        * demo is subscribing to the topic to which no one is publishing, probability
        * of receiving publish message before subscribe ack is zero; but application
        * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
        * receive packet from network. */
      mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

      if( mqttStatus != MQTTSuccess )
      {
          returnStatus = EXIT_FAILURE;
          LogError( ( "MQTT_ProcessLoop returned with status = %s.",
                      MQTT_Status_strerror( mqttStatus ) ) );
      }
  }

  /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
    * in eventCallback to reflect the status of the SUBACK sent by the broker. */
  if( ( returnStatus == EXIT_SUCCESS ) && ( globalSubAckStatus == MQTTSubAckFailure ) )
  {
      /* If server rejected the subscription request, attempt to resubscribe to topic.
        * Attempts are made according to the exponential backoff retry strategy
        * implemented in retryUtils. */
      LogInfo( ( "Server rejected initial subscription request. Attempting to re-subscribe to topic %.*s.",
                  MQTT_EXAMPLE_TOPIC_LENGTH,
                  MQTT_EXAMPLE_TOPIC ) );
      returnStatus = handleResubscribe( pMqttContext );
  }

  return returnStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief 
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t u16TestData[] = {4,5,6};
int Publish(MQTTContext_t * pMqttContext, bool * pClientSessionPresent)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;

    LogInfo( ( "Sending Publish to the MQTT topic %.*s.",
                MQTT_EXAMPLE_TOPIC_LENGTH,
                MQTT_EXAMPLE_TOPIC ) );


    returnStatus = publishToTopic( pMqttContext, u16TestData);

    /* Calling MQTT_ProcessLoop to process incoming publish echo, since
    * application subscribed to the same topic the broker will send
    * publish message back to the application. This function also
    * sends ping request to broker if MQTT_KEEP_ALIVE_INTERVAL_SECONDS
    * has expired since the last MQTT packet sent and receive
    * ping responses. */
    mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

    /* For any error in #MQTT_ProcessLoop, exit the loop and disconnect
    * from the broker. */
    if( mqttStatus != MQTTSuccess )
    {
        LogError( ( "MQTT_ProcessLoop returned with status = %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
        returnStatus = EXIT_FAILURE;
        // break;
    }

    return returnStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief 
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
int DisconnectMQTT(MQTTContext_t * pMqttContext, bool * pClientSessionPresent)
{
  int returnStatus = EXIT_SUCCESS;
  MQTTStatus_t mqttStatus = MQTTSuccess;

  /* Unsubscribe from the topic. */
  LogInfo( ( "Unsubscribing from the MQTT topic %.*s.",
              MQTT_EXAMPLE_TOPIC_LENGTH,
              MQTT_EXAMPLE_TOPIC ) );
  returnStatus = unsubscribeFromTopic( pMqttContext );

  if( returnStatus == EXIT_SUCCESS )
  {
      /* Process Incoming UNSUBACK packet from the broker. */
      mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

      if( mqttStatus != MQTTSuccess )
      {
          returnStatus = EXIT_FAILURE;
          LogError( ( "MQTT_ProcessLoop returned with status = %s.",
                      MQTT_Status_strerror( mqttStatus ) ) );
      }
  }

  /* Send an MQTT Disconnect packet over the already connected TCP socket.
    * There is no corresponding response for the disconnect packet. After sending
    * disconnect, client must close the network connection. */
  if( mqttSessionEstablished == true )
  {
      LogInfo( ( "Disconnecting the MQTT connection with %.*s.",
                  AWS_IOT_ENDPOINT_LENGTH,
                  AWS_IOT_ENDPOINT ) );

      if( returnStatus == EXIT_FAILURE )
      {
          /* Returned status is not used to update the local status as there
            * were failures in demo execution. */
          ( void ) disconnectMqttSession( pMqttContext );
      }
      else
      {
          mqttSessionEstablished = false;
          returnStatus = disconnectMqttSession( pMqttContext );
      }
  }

  /* Reset global SUBACK status variable after completion of subscription request cycle. */
  globalSubAckStatus = MQTTSubAckFailure;

  return returnStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize AWS_MQTT
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void D_AWS_MQTT_Init(void)
{
  
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());


    int returnStatus = EXIT_SUCCESS;
    // MQTTContext_t mqttContext = { 0 };
    // NetworkContext_t xNetworkContext = { 0 };
    bool clientSessionPresent = false;
    struct timespec tp;

    /* Seed pseudo random number generator (provided by ISO C standard library) for
     * use by retry utils library when retrying failed network operations. */

    /* Get current time to seed pseudo random number generator. */
    ( void ) clock_gettime( CLOCK_REALTIME, &tp );
    /* Seed pseudo random number generator with nanoseconds. */
    srand( tp.tv_nsec );

    /* Initialize MQTT library. Initialization of the MQTT library needs to be
     * done only once in this demo. */
    returnStatus = initializeMqtt( &mqttContext, &xNetworkContext );
}

// END OF FILE