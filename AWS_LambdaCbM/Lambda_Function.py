import datetime
import boto3
import numpy as np

def lambda_handler(event, context):
    # print(event)

    aws_access_key_id       = "AKIASNYUHESZAVGDN4WN"
    aws_secret_access_key   = "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"
    
    BUCKET  = "iiotpredictivebucket1"
    FFT_KEY = "Vibration_Freq"
    
    object_time = datetime.datetime.fromtimestamp(event['Time'])
    date_time   = object_time.strftime("%d/%m/%Y %H:%M:%S")
    
    KEY_date_time   = object_time.strftime("%d_%m_%Y_%H_%M_%S/")
    KEY_CSV         = ".csv"
    
    # Number of samples in normalized_tone
    N           = 128
    SAMPLE_RATE = 125

    X_Axis  = np.array([event["X0"],event["X1"],event["X2"],event["X3"],event["X4"],event["X5"],event["X6"],event["X7"],event["X8"],event["X9"],event["X10"],event["X11"],event["X12"],event["X13"],event["X14"],event["X15"],event["X16"],event["X17"],event["X18"],event["X19"],event["X20"],event["X21"],event["X22"],event["X23"],event["X24"],event["X25"],event["X26"],event["X27"],event["X28"],event["X29"],event["X30"],event["X31"],event["X32"],event["X33"],event["X34"],event["X35"],event["X36"],event["X37"],event["X38"],event["X39"],event["X40"],event["X41"],event["X42"],event["X43"],event["X44"],event["X45"],event["X46"],event["X47"],event["X48"],event["X49"],event["X50"],event["X51"],event["X52"],event["X53"],event["X54"],event["X55"],event["X56"],event["X57"],event["X58"],event["X59"],event["X60"],event["X61"],event["X62"],event["X63"],event["X64"],event["X65"],event["X66"],event["X67"],event["X68"],event["X69"],event["X70"],event["X71"],event["X72"],event["X73"],event["X74"],event["X75"],event["X76"],event["X77"],event["X78"],event["X79"],event["X80"],event["X81"],event["X82"],event["X83"],event["X84"],event["X85"],event["X86"],event["X87"],event["X88"],event["X89"],event["X90"],event["X91"],event["X92"],event["X93"],event["X94"],event["X95"],event["X96"],event["X97"],event["X98"],event["X99"],event["X100"],event["X101"],event["X102"],event["X103"],event["X104"],event["X105"],event["X106"],event["X107"],event["X108"],event["X109"],event["X110"],event["X111"],event["X112"],event["X113"],event["X114"],event["X115"],event["X116"],event["X117"],event["X118"],event["X119"],event["X120"],event["X121"],event["X122"],event["X123"],event["X124"],event["X125"],event["X126"],event["X127"]])
    Y_Axis  = np.array([event["Y0"],event["Y1"],event["Y2"],event["Y3"],event["Y4"],event["Y5"],event["Y6"],event["Y7"],event["Y8"],event["Y9"],event["Y10"],event["Y11"],event["Y12"],event["Y13"],event["Y14"],event["Y15"],event["Y16"],event["Y17"],event["Y18"],event["Y19"],event["Y20"],event["Y21"],event["Y22"],event["Y23"],event["Y24"],event["Y25"],event["Y26"],event["Y27"],event["Y28"],event["Y29"],event["Y30"],event["Y31"],event["Y32"],event["Y33"],event["Y34"],event["Y35"],event["Y36"],event["Y37"],event["Y38"],event["Y39"],event["Y40"],event["Y41"],event["Y42"],event["Y43"],event["Y44"],event["Y45"],event["Y46"],event["Y47"],event["Y48"],event["Y49"],event["Y50"],event["Y51"],event["Y52"],event["Y53"],event["Y54"],event["Y55"],event["Y56"],event["Y57"],event["Y58"],event["Y59"],event["Y60"],event["Y61"],event["Y62"],event["Y63"],event["Y64"],event["Y65"],event["Y66"],event["Y67"],event["Y68"],event["Y69"],event["Y70"],event["Y71"],event["Y72"],event["Y73"],event["Y74"],event["Y75"],event["Y76"],event["Y77"],event["Y78"],event["Y79"],event["Y80"],event["Y81"],event["Y82"],event["Y83"],event["Y84"],event["Y85"],event["Y86"],event["Y87"],event["Y88"],event["Y89"],event["Y90"],event["Y91"],event["Y92"],event["Y93"],event["Y94"],event["Y95"],event["Y96"],event["Y97"],event["Y98"],event["Y99"],event["Y100"],event["Y101"],event["Y102"],event["Y103"],event["Y104"],event["Y105"],event["Y106"],event["Y107"],event["Y108"],event["Y109"],event["Y110"],event["Y111"],event["Y112"],event["Y113"],event["Y114"],event["Y115"],event["Y116"],event["Y117"],event["Y118"],event["Y119"],event["Y120"],event["Y121"],event["Y122"],event["Y123"],event["Y124"],event["Y125"],event["Y126"],event["Y127"]])
    Z_Axis  = np.array([event["Z0"],event["Z1"],event["Z2"],event["Z3"],event["Z4"],event["Z5"],event["Z6"],event["Z7"],event["Z8"],event["Z9"],event["Z10"],event["Z11"],event["Z12"],event["Z13"],event["Z14"],event["Z15"],event["Z16"],event["Z17"],event["Z18"],event["Z19"],event["Z20"],event["Z21"],event["Z22"],event["Z23"],event["Z24"],event["Z25"],event["Z26"],event["Z27"],event["Z28"],event["Z29"],event["Z30"],event["Z31"],event["Z32"],event["Z33"],event["Z34"],event["Z35"],event["Z36"],event["Z37"],event["Z38"],event["Z39"],event["Z40"],event["Z41"],event["Z42"],event["Z43"],event["Z44"],event["Z45"],event["Z46"],event["Z47"],event["Z48"],event["Z49"],event["Z50"],event["Z51"],event["Z52"],event["Z53"],event["Z54"],event["Z55"],event["Z56"],event["Z57"],event["Z58"],event["Z59"],event["Z60"],event["Z61"],event["Z62"],event["Z63"],event["Z64"],event["Z65"],event["Z66"],event["Z67"],event["Z68"],event["Z69"],event["Z70"],event["Z71"],event["Z72"],event["Z73"],event["Z74"],event["Z75"],event["Z76"],event["Z77"],event["Z78"],event["Z79"],event["Z80"],event["Z81"],event["Z82"],event["Z83"],event["Z84"],event["Z85"],event["Z86"],event["Z87"],event["Z88"],event["Z89"],event["Z90"],event["Z91"],event["Z92"],event["Z93"],event["Z94"],event["Z95"],event["Z96"],event["Z97"],event["Z98"],event["Z99"],event["Z100"],event["Z101"],event["Z102"],event["Z103"],event["Z104"],event["Z105"],event["Z106"],event["Z107"],event["Z108"],event["Z109"],event["Z110"],event["Z111"],event["Z112"],event["Z113"],event["Z114"],event["Z115"],event["Z116"],event["Z117"],event["Z118"],event["Z119"],event["Z120"],event["Z121"],event["Z122"],event["Z123"],event["Z124"],event["Z125"],event["Z126"],event["Z127"]])
    
    
    RFFT_X_Axis  = np.fft.rfft(X_Axis)
    RFFT_Y_Axis  = np.fft.rfft(Y_Axis)
    RFFT_Z_Axis  = np.fft.rfft(Z_Axis)
    
    RFFT_ABS_X_Axis = np.abs(RFFT_X_Axis)/100000
    RFFT_ABS_Y_Axis = np.abs(RFFT_Y_Axis)/100000
    RFFT_ABS_Z_Axis = np.abs(RFFT_Z_Axis)/100000

    RFFT_Freq_Axis = np.fft.fftfreq(N, 1 / SAMPLE_RATE)

    FFT_Data = {}
    
    dl = ';'
    nl = '\r\n'
    
    LOW_FQ  = 4
    
    str_RFFT_Freq_Axis = (str(RFFT_Freq_Axis[LOW_FQ]).strip('()'))
        
    str_X_Axis = (str(RFFT_ABS_X_Axis[LOW_FQ]).strip('()'))
    str_Y_Axis = (str(RFFT_ABS_Y_Axis[LOW_FQ]).strip('()'))
    str_Z_Axis = (str(RFFT_ABS_Z_Axis[LOW_FQ]).strip('()'))

    FFT_Data['Body'] = str_RFFT_Freq_Axis + dl + str_X_Axis + dl + str_Y_Axis + dl + str_Z_Axis + nl
    
        
    for x in range(64-LOW_FQ-1):
        str_RFFT_Freq_Axis = (str(RFFT_Freq_Axis[x+LOW_FQ+1]).strip('()'))
        
        str_X_Axis = (str(RFFT_ABS_X_Axis[x+LOW_FQ+1]).strip('()'))
        str_Y_Axis = (str(RFFT_ABS_Y_Axis[x+LOW_FQ+1]).strip('()'))
        str_Z_Axis = (str(RFFT_ABS_Z_Axis[x+LOW_FQ+1]).strip('()'))
        
        FFT_Data['Body'] += str_RFFT_Freq_Axis + dl + str_X_Axis + dl + str_Y_Axis + dl + str_Z_Axis + nl

    # print(FFT_Data)
    
    # Write object back to S3 Object Lambda
    s3 = boto3.client('s3', 
        aws_access_key_id = aws_access_key_id, 
        aws_secret_access_key = aws_secret_access_key)
        
    response = s3.put_object(Bucket=BUCKET, Key=KEY_date_time+FFT_KEY+KEY_CSV, **FFT_Data)
    
    ############################################################################
    
    Vibration_Data  = {}
    Vibration_KEY   = "Vibration_Time"
    
    str_Time_Axis = str(0)
        
    str_X_Axis = str(X_Axis[0]/100000)
    str_Y_Axis = str(Y_Axis[0]/100000)
    str_Z_Axis = str(Z_Axis[0]/100000)
    
    Vibration_Data['Body'] = str_Time_Axis + dl + str_X_Axis + dl + str_Y_Axis + dl + str_Z_Axis + nl
    
        
    for x in range(128-1):
        str_Time_Axis = str(x+1)
        
        str_X_Axis = str(X_Axis[x+1]/100000)
        str_Y_Axis = str(Y_Axis[x+1]/100000)
        str_Z_Axis = str(Z_Axis[x+1]/100000)
        
        Vibration_Data['Body'] += str_Time_Axis + dl + str_X_Axis + dl + str_Y_Axis + dl + str_Z_Axis + nl
    
    # Write object back to S3 Object Lambda
    s3 = boto3.client('s3', 
        aws_access_key_id = aws_access_key_id, 
        aws_secret_access_key = aws_secret_access_key)
        
    response = s3.put_object(Bucket=BUCKET, Key=KEY_date_time+Vibration_KEY+KEY_CSV, **Vibration_Data)
    
    
    
    ############################################################################
    
    Harm_Marg_Fac   = 1

    # 1. Harmonic    
    Harm1_Offset    = LOW_FQ+7
    
    Harm1_Mes_X     = 0.84
    Harm1_Mes_Y     = 0.48
    Harm1_Mes_Z     = 2.82
    
    Harm1_Max_X     = Harm1_Mes_X+(Harm_Marg_Fac*Harm1_Mes_X)
    Harm1_Max_Y     = Harm1_Mes_Y+(Harm_Marg_Fac*Harm1_Mes_Y)
    Harm1_Max_Z     = Harm1_Mes_Z+(Harm_Marg_Fac*Harm1_Mes_Z)
    
    # 2. Harmonic
    Harm2_Offset    = LOW_FQ+22
    
    Harm2_Mes_X     = 1.24
    Harm2_Mes_Y     = 0.73
    Harm2_Mes_Z     = 2.62
    
    Harm2_Max_X     = Harm2_Mes_X+(Harm_Marg_Fac*Harm2_Mes_X)
    Harm2_Max_Y     = Harm2_Mes_Y+(Harm_Marg_Fac*Harm2_Mes_Y)
    Harm2_Max_Z     = Harm2_Mes_Z+(Harm_Marg_Fac*Harm2_Mes_Z)
    
    # 3. Harmonic
    Harm3_Offset    = LOW_FQ+37
    
    Harm3_Mes_X     = 1.63
    Harm3_Mes_Y     = 1.33
    Harm3_Mes_Z     = 4.37
    
    Harm3_Max_X     = Harm3_Mes_X+(Harm_Marg_Fac*Harm3_Mes_X)
    Harm3_Max_Y     = Harm3_Mes_Y+(Harm_Marg_Fac*Harm3_Mes_Y)
    Harm3_Max_Z     = Harm3_Mes_Z+(Harm_Marg_Fac*Harm3_Mes_Z)
    
    # 4. Harmonic
    Harm4_Offset    = LOW_FQ+51
    
    Harm4_Mes_X     = 1.32
    Harm4_Mes_Y     = 0.87
    Harm4_Mes_Z     = 2.63
    
    Harm4_Max_X     = Harm4_Mes_X+(Harm_Marg_Fac*Harm4_Mes_X)
    Harm4_Max_Y     = Harm4_Mes_Y+(Harm_Marg_Fac*Harm4_Mes_Y)
    Harm4_Max_Z     = Harm4_Mes_Z+(Harm_Marg_Fac*Harm4_Mes_Z)
    
    # CbM
    Harm1_X_Status    = 0
    Harm2_X_Status    = 0
    Harm3_X_Status    = 0
    Harm4_X_Status    = 0
    
    Harm1_Y_Status    = 0
    Harm2_Y_Status    = 0
    Harm3_Y_Status    = 0
    Harm4_Y_Status    = 0
    
    Harm1_Z_Status    = 0
    Harm2_Z_Status    = 0
    Harm3_Z_Status    = 0
    Harm4_Z_Status    = 0
    
    for x in range(5):
        # 1. Harmonic
        if RFFT_ABS_X_Axis[Harm1_Offset+x] >= Harm1_Max_X:
            Harm1_X_Status = 1
        if RFFT_ABS_Y_Axis[Harm1_Offset+x] >= Harm1_Max_Y:
            Harm1_Y_Status = 1
        if RFFT_ABS_Z_Axis[Harm1_Offset+x] >= Harm1_Max_Z:
            Harm1_Z_Status = 1
        
        # 2. Harmonic
        if RFFT_ABS_X_Axis[Harm2_Offset+x] >= Harm2_Max_X:
            Harm2_X_Status = 1
        if RFFT_ABS_Y_Axis[Harm2_Offset+x] >= Harm2_Max_Y:
            Harm2_Y_Status = 1
        if RFFT_ABS_Z_Axis[Harm2_Offset+x] >= Harm2_Max_Z:
            Harm2_Z_Status = 1
            
        # 3. Harmonic
        if RFFT_ABS_X_Axis[Harm3_Offset+x] >= Harm3_Max_X:
            Harm3_X_Status = 1
        if RFFT_ABS_Y_Axis[Harm3_Offset+x] >= Harm3_Max_Y:
            Harm3_Y_Status = 1
        if RFFT_ABS_Z_Axis[Harm3_Offset+x] >= Harm3_Max_Z:
            Harm3_Z_Status = 1
            
        # 4. Harmonic
        if RFFT_ABS_X_Axis[Harm4_Offset+x] >= Harm4_Max_X:
            Harm4_X_Status = 1
        if RFFT_ABS_Y_Axis[Harm4_Offset+x] >= Harm4_Max_Y:
            Harm4_Y_Status = 1
        if RFFT_ABS_Z_Axis[Harm4_Offset+x] >= Harm4_Max_Z:
            Harm4_Z_Status = 1
    
    if Harm1_X_Status+Harm1_Y_Status+Harm1_Z_Status+Harm2_X_Status+Harm2_Y_Status+Harm2_Z_Status+Harm3_X_Status+Harm3_Y_Status+Harm3_Z_Status+Harm4_X_Status+Harm4_Y_Status+Harm4_Z_Status != 0:
        SNS_Msg = {}
        
        SNS_Msg =  "IIoT Predictive Maintenance Unit 01" + nl
        SNS_Msg += "Timestamp:" + nl
        SNS_Msg += date_time + nl
        SNS_Msg += "Errors:" + nl
        
        # 1. Harmonic
        if Harm1_X_Status == 1:
            SNS_Msg += "X-Axis 1. Harmonic Error" + nl
        if Harm1_Y_Status == 1:
            SNS_Msg += "Y-Axis 1. Harmonic Error" + nl
        if Harm1_Z_Status == 1:
            SNS_Msg += "Z-Axis 1. Harmonic Error" + nl
            
        # 2. Harmonic
        if Harm2_X_Status == 1:
            SNS_Msg += "X-Axis 2. Harmonic Error" + nl
        if Harm2_Y_Status == 1:
            SNS_Msg += "Y-Axis 2. Harmonic Error" + nl
        if Harm2_Z_Status == 1:
            SNS_Msg += "Z-Axis 2. Harmonic Error" + nl
            
        # 3. Harmonic
        if Harm3_X_Status == 1:
            SNS_Msg += "X-Axis 3. Harmonic Error" + nl
        if Harm3_Y_Status == 1:
            SNS_Msg += "Y-Axis 3. Harmonic Error" + nl
        if Harm3_Z_Status == 1:
            SNS_Msg += "Z-Axis 3. Harmonic Error" + nl
            
        # 4. Harmonic
        if Harm4_X_Status == 1:
            SNS_Msg += "X-Axis 4. Harmonic Error" + nl
        if Harm4_Y_Status == 1:
            SNS_Msg += "Y-Axis 4. Harmonic Error" + nl
        if Harm4_Z_Status == 1:
            SNS_Msg += "Z-Axis 4. Harmonic Error" + nl
    
    
        client = boto3.client('sns')
        response = client.publish (
            TargetArn = "arn:aws:sns:eu-central-1:167009264818:IIoT_Predictive_Error",
            Subject = "Predictive Unit 01 Warning",
            Message = SNS_Msg,
            MessageStructure = 'string'
        )    