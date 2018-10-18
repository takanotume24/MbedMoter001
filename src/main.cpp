#include <mbed.h>
#include <stdlib.h>

#define RUN_MODE_12     400
#define RUN_MODE_2      401
#define TEST_MODE       402
#define RUN_MODE_3      403
#define RUN_MODE_23     404

#define FORWARD         100
#define BACK            101
#define RIGHT           102
#define LEFT            103
#define STOP            104
#define TURN_RIGHT      105
#define TURN_LEFT       106

#define ON_WHITE        200
#define ON_BLACK        201

#define ON_STRAIGHT_WAY 300
#define ON_CURVE_WAY    301

#define ON_WAY          500
#define MISSING         501
#define ALL_WAY         502
#define BEFORE_WAY_IS_ALL_BLACK   503

#define HIGH            1
#define LOW             0
#define SENSOR_TARGET_VAL 1.0

int histry[10];
Ticker moterTimerRight;
Ticker moterTimerLeft;
Ticker sensorTimer;
BusOut moterRight(PB_15,PB_13,PB_14,PC_4);
BusOut moterLeft(PC_12,PB_7,PA_15,PC_13);

BusIn digitalSwitch(PD_2,PC_11,PC_9,PB_8);

DigitalOut SA(PB_15);
DigitalOut SB(PB_13);
DigitalOut SA_(PB_14);
DigitalOut SB_(PC_4);
DigitalOut ledLeft(PC_10);
DigitalOut ledCenter(PC_6);
DigitalOut ledRight(PC_8);
AnalogIn SensorLeft(PC_2);
AnalogIn SensorRight(PB_1);
AnalogIn SensorCenter(PC_3);
AnalogIn volume1(PC_5);
AnalogIn volume2(PA_0);
AnalogIn volume3(PA_1);
Serial pc(USBTX, USBRX, 115200);

typedef struct{
    float diff[2];
    float integral;
    float p,i,d;
}RIGHT_VAL;

typedef struct{
    float diff[2];
    float integral;
    float p,i,d;
}LEFT_VAL;

typedef struct{
    int flag;
    int newDirection;
    int oldDirection;
    int direction;
    float valueOfSensorLeft;
    float valueOfSensorCenter;
    float valueOfSensorRight;
    float valueOfVolume1;
    float valueOfVolume2;
    float valueOfVolume3;
    int brightnessLeft;
    int brightnessCenter;
    int brightnessRight;
    int beforeDirection;
    int mode;
    int onWay;
    int beforeWayStatus;
    float controlLeftVal;
    float controlRightVal;
}DATA;

typedef struct{
    float speedLow;
    float speedMedium;
    float speedHigh;
    float Threshold;
    bool DEBUG;
    float KP;
    float KI;
    float KD;
}VALUE;

typedef struct{
    int directionRight;
    int directionLeft;
    float speedRight;
    float speedLeft;
}MOTER;

DATA data;
VALUE value;
MOTER moter;
RIGHT_VAL rightVal;
LEFT_VAL leftVal;



void moter_2_StraightLeft();
void moter_2_StraightRight();
void getData();
void setDirection();
void start();
void goLeft();
void goRight();
void goStaight();
void stop();
void setBrightness();
void moter_12_StraightRight();
void moter_12_StraightLeft();
void turnLeft();
void turnRight();
void modeSet();
void moter_3_StraightLeft();
void moter_3_StraightRight();
void moter_23_StraightLeft();
void moter_23_StraightRight();
float limit(float);

int countRight = 0;
int countLeft = 0;

float pidCalcLeft(float sensorVal, float targetVal, float speed){
    leftVal.diff[0] = leftVal.diff[1];
    leftVal.diff[1] = sensorVal - targetVal;
    leftVal.integral = leftVal.integral +(leftVal.diff[1]+leftVal.diff[0])/2.0*speed;

    leftVal.p = value.KP * leftVal.diff[1];
    leftVal.i = value.KI * leftVal.integral;
    leftVal.d = value.KD * (leftVal.diff[1]-leftVal.diff[0])/speed;

    return abs(leftVal.p+leftVal.i+leftVal.p);
}


float pidCalcRight(float sensorVal, float targetVal, float speed){
    rightVal.diff[0] = rightVal.diff[1];
    rightVal.diff[1] = sensorVal - targetVal;
    rightVal.integral = rightVal.integral +(rightVal.diff[1]+rightVal.diff[0])/2.0*speed;

    rightVal.p = 10*value.KP * rightVal.diff[1];
    rightVal.i = 10*value.KI * rightVal.integral;
    rightVal.d = 10*value.KD * (rightVal.diff[1]-rightVal.diff[0])/speed;

    return abs(rightVal.p+rightVal.i+rightVal.p);
}

float limit(float val){
    if(val > 1){
        return 1.0;
    }
    if(val < 0){
        return 0.0;
    }

    return 0;
}

void getData(){
    data.valueOfSensorLeft = SensorLeft.read();
    data.valueOfSensorCenter = SensorCenter.read();
    data.valueOfSensorRight = SensorRight.read();
    data.valueOfVolume1 = volume1.read();
    data.valueOfVolume2 = volume2.read();
    data.valueOfVolume3 = volume3.read();
    setBrightness();
}

void setBrightness(){
    if(data.valueOfSensorLeft < 0.5){
        data.brightnessLeft = ON_WHITE;
        ledLeft = HIGH;
    }else{
        data.brightnessLeft = ON_BLACK;
        ledLeft = LOW;
    }

    if(data.valueOfSensorRight < 0.5){
        data.brightnessRight = ON_WHITE;
        ledRight = HIGH;
    }else{
        data.brightnessRight = ON_BLACK;
        ledRight = LOW;
    }

    if(data.valueOfSensorCenter < 0.5){
        data.brightnessCenter = ON_WHITE;
        ledCenter = HIGH;
    }else{
        data.brightnessCenter = ON_BLACK;
        ledCenter = LOW;
    }

}

void setSpeed(){
    data.controlLeftVal = pidCalcLeft(data.valueOfSensorLeft, SENSOR_TARGET_VAL, moter.speedLeft);
    data.controlRightVal = pidCalcRight(data.valueOfSensorRight, SENSOR_TARGET_VAL, moter.speedRight);
    switch(data.direction){
        case FORWARD:
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
            break;

        case TURN_RIGHT:
            switch(data.onWay){
                case ON_WAY:
                    moter.speedLeft = value.speedHigh ;
                    moter.speedRight = value.speedHigh * data.controlLeftVal ; 
                    break;

                case MISSING:
                    moter.speedLeft = value.speedHigh;
                    moter.speedRight = value.speedHigh;
                    break;
            }
            break;

        case RIGHT:
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh  * data.controlLeftVal ; 
                break;

        case TURN_LEFT:
            switch(data.onWay){
                case ON_WAY:
                    moter.speedLeft = value.speedHigh * data.controlRightVal;
                    moter.speedRight = value.speedHigh ; 
                    break;

                case MISSING:
                    moter.speedLeft = value.speedHigh;
                    moter.speedLeft = value.speedHigh;
                    break;
            }
            break;

        case LEFT:
                moter.speedLeft = value.speedHigh * data.controlRightVal;
                moter.speedRight = value.speedHigh;
                break;

    }
}
void setDirection(){
    data.oldDirection = data.direction;
    switch(data.brightnessCenter){
        case ON_BLACK:  //真ん中が線上
            data.onWay = ON_WAY;
            if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK ){
                // data.newDirection = FORWARD;
                data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
            }
            if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
                // data.newDirection = FORWARD;
                data.beforeWayStatus = ON_WAY;
            }
            if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
                switch(data.beforeWayStatus){
                    case BEFORE_WAY_IS_ALL_BLACK:
                        data.newDirection = RIGHT;
                        data.beforeDirection = RIGHT;
                        data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
                        break;

                    case ON_WAY:
                        data.newDirection = LEFT;
                        data.beforeDirection = LEFT;
                        data.beforeWayStatus = ON_WAY;
                        break;
                }
            }
            if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
                switch(data.beforeWayStatus){
                    case BEFORE_WAY_IS_ALL_BLACK:
                        data.newDirection = LEFT;
                        data.beforeDirection = LEFT;
                        data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
                        break;

                    case ON_WAY:
                        data.newDirection = RIGHT;
                        data.beforeDirection = RIGHT;
                        data.beforeWayStatus = ON_WAY;
                        break;
                }
            }
            break;
         
         case ON_WHITE:  //真ん中が線上にない
            if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
                data.onWay = MISSING;
                switch(data.beforeWayStatus){
                    case BEFORE_WAY_IS_ALL_BLACK:
                        switch(data.beforeDirection){
                            case LEFT:
                                data.newDirection = TURN_RIGHT;
                                break;

                            case RIGHT:
                                data.newDirection = TURN_LEFT;
                                break;
                        }
                        data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
                        break;
                    
                    case ON_WAY:
                        switch(data.direction){
                            case LEFT:
                                data.newDirection = TURN_LEFT;
                                break;

                            case RIGHT:
                                data.newDirection = TURN_RIGHT;
                                break;
                        }
                        data.beforeWayStatus = MISSING;
                        break;
                        

                    case MISSING:
                        switch(data.direction){
                            case LEFT:
                                data.newDirection = TURN_LEFT;
                                break;

                            case RIGHT:
                                data.newDirection = TURN_RIGHT;
                                break;
                        }
                        data.beforeWayStatus = MISSING;
                        break;
                }
            }
            if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
                data.onWay = ON_WAY;
                switch(data.beforeWayStatus){
                    case BEFORE_WAY_IS_ALL_BLACK:
                        data.beforeWayStatus = ON_WAY;
                        break;
                    
                    case ON_WAY:
                    case MISSING:
                        data.beforeDirection = LEFT;
                        data.newDirection = LEFT;
                        data.beforeWayStatus = ON_WAY;
                        break;
                }
            }
            if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
                data.onWay = ON_WAY;
                switch(data.beforeWayStatus){
                    case BEFORE_WAY_IS_ALL_BLACK:
                        data.beforeWayStatus = ON_WAY;
                        break;
                    
                    case ON_WAY:
                    case MISSING:
                        data.beforeDirection = RIGHT;
                        data.newDirection = RIGHT;
                        data.beforeWayStatus = ON_WAY;
                        break;
                }
            }
            if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK){
                data.onWay = ON_WAY;
                data.newDirection = FORWARD;
                data.beforeWayStatus = ON_WAY;
            }
            break;

    }
    if(data.brightnessCenter == ON_BLACK){
        
    }
    if(data.brightnessCenter == ON_WHITE){
        
    }

    if(data.newDirection == data.oldDirection){
        data.flag = 0;
    }else{
        data.direction = data.newDirection;
        data.flag = 1;
    }
}

void start(){
    setDirection();
    setSpeed();
    if(data.flag){
        switch(data.direction){
            case FORWARD:
                goStaight();
                break;

            case RIGHT:
                goRight();
                break;

            case LEFT:
                goLeft();
                break;
            
            case STOP:
                stop();
                break;
            
            case TURN_LEFT:
                turnLeft();
                break;

            case TURN_RIGHT:
                turnRight();
                break;

            default:
                stop();
                break;
        }
    }
}

void startMoterLeft(){
    switch(data.mode){
        case TEST_MODE:
            moterTimerLeft.attach(&moter_2_StraightLeft, moter.speedLeft);
            break;
        case RUN_MODE_12:
            moterTimerLeft.attach(&moter_12_StraightLeft, moter.speedLeft);
            break;
        case RUN_MODE_2:
            moterTimerLeft.attach(&moter_2_StraightLeft, moter.speedLeft);
            break;
        case RUN_MODE_3:
            moterTimerLeft.attach(&moter_3_StraightLeft, moter.speedLeft);
            break;
        case RUN_MODE_23:
            moterTimerLeft.attach(&moter_23_StraightLeft, moter.speedLeft);
            break;
    }
}
void startMoterRight(){
    switch(data.mode){
        case TEST_MODE :
            moterTimerRight.attach(&moter_2_StraightRight, moter.speedRight);
            break;
        case RUN_MODE_12:
            moterTimerRight.attach(&moter_12_StraightRight, moter.speedRight);
            break;
        case RUN_MODE_2:
            moterTimerRight.attach(&moter_2_StraightRight, moter.speedRight);
            break;
        case RUN_MODE_3:
            moterTimerRight.attach(&moter_3_StraightRight, moter.speedRight);
            break;
        case RUN_MODE_23:
            moterTimerRight.attach(&moter_23_StraightRight, moter.speedRight);
    }
    moterTimerRight.attach(&moter_2_StraightRight, moter.speedRight);
}

void stopMoterLeft(){
    moterTimerLeft.detach();
}
void stopMoterRight(){
    moterTimerRight.detach();
}

void goStaight(){
    moter.directionLeft = FORWARD;
    moter.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}

void goRight(){
    moter.directionLeft = FORWARD;
    moter.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}
void goLeft(){
    moter.directionLeft = FORWARD;
    moter.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}
void turnRight(){
    moter.directionLeft = FORWARD;
    moter.directionRight = FORWARD;
    startMoterLeft();
    stopMoterRight();
}
void turnLeft(){
    moter.directionLeft = FORWARD;
    moter.directionRight = FORWARD;
    stopMoterLeft();
    startMoterRight();
}
void stop(){
    stopMoterLeft();
    stopMoterRight();    
}


void moter_2_StraightLeft(){

    if(moter.directionLeft== FORWARD){
        if(countLeft == 3){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(moter.directionLeft == BACK){
        if(countRight == 0){
            countLeft = 3;
        }else{
            countLeft--;
        }
    }

    switch(countLeft){
        case 0:
            moterLeft = 0b1001;
            break;
         
        case 1:
            moterLeft = 0b1100;
            break;

        case 2:
            moterLeft = 0b0110;
            break;

        case 3:
            moterLeft = 0b0011;
            break;
            
    }
    
}

void moter_2_StraightRight(){
    // for(int i = 0.1; i < 0.01 ; i=i-0.01){
    //     moter.speedRight = i;
    //     moter_2_StraightRight();
    // }

    if(moter.directionRight == FORWARD){
        if(countRight == 3){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(moter.directionRight == BACK){
        if(countRight == 0){
            countRight = 3;
        }else{
            countRight--;
        }
    }   
    switch(countRight){
        case 0:
            moterRight = 0b0011;
            break;

        case 1:
            moterRight = 0b0110;
            break;

        case 2:
            moterRight = 0b1100;
            break;

        case 3:
            moterRight = 0b1001;
            break;

        default :
            break;
    }
}
void moter_3_StraightLeft(){

    if(moter.directionLeft== FORWARD){
        if(countLeft == 3){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(moter.directionLeft == BACK){
        if(countRight == 0){
            countLeft = 3;
        }else{
            countLeft--;
        }
    }

    switch(countLeft){
        case 0:
            moterLeft = 0b0111;
            break;
         
        case 1:
            moterLeft = 0b1011;
            break;

        case 2:
            moterLeft = 0b1101;
            break;

        case 3:
            moterLeft = 0b1110;
            break;
            
    }
    
}

void moter_3_StraightRight(){
    if(moter.directionRight == FORWARD){
        if(countRight == 3){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(moter.directionRight == BACK){
        if(countRight == 0){
            countRight = 3;
        }else{
            countRight--;
        }
    }   
    switch(countRight){
        case 0:
            moterRight = 0b1110;
            break;

        case 1:
            moterRight = 0b1101;
            break;

        case 2:
            moterRight = 0b1011;
            break;

        case 3:
            moterRight = 0b0111;
            break;

        default :
            break;
    }
}
void moter_12_StraightLeft(){
    if(moter.directionLeft == FORWARD){
        if(countLeft == 7){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(moter.directionLeft == BACK){
        if(countLeft == 0){
            countLeft = 7;
        }else{
            countLeft--;
        }
    }  
    switch(countLeft){
        case 0:
            moterLeft = 0b1001;
            break;

        case 1:
            moterLeft = 0b1000;
            break;

        case 2:
            moterLeft = 0b1100;
            break;

        case 3:
            moterLeft = 0b0100;
            break;

        case 4:
            moterLeft = 0b0110;
            break;
        
        case 5:
            moterLeft = 0b0010;
            break;

        case 6:
            moterLeft = 0b0011;
            break;

        case 7:
            moterLeft = 0b0001;
            break;

    }
}


void moter_12_StraightRight(){
    if(moter.directionRight == FORWARD){
        if(countRight == 7){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(moter.directionRight == BACK){
        if(countRight == 0){
            countRight = 7;
        }else{
            countRight--;
        }
    }  
    switch(countRight){
        case 0:
            moterRight = 0b0001;
            break;

        case 1:
            moterRight = 0b0011;
            break;

        case 2:
            moterRight = 0b0010;
            break;

        case 3:
            moterRight = 0b0110;
            break;

        case 4:
            moterRight = 0b0100;
            break;
        
        case 5:
            moterRight = 0b1100;
            break;

        case 6:
            moterRight = 0b1000;
            break;

        case 7:
            moterRight = 0b1001;
            break;

    }
}
void moter_23_StraightLeft(){
    if(moter.directionLeft == FORWARD){
        if(countLeft == 7){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(moter.directionLeft == BACK){
        if(countLeft == 0){
            countLeft = 7;
        }else{
            countLeft--;
        }
    }  
    switch(countLeft){
        case 0:
            moterLeft = 0b1001;
            break;

        case 1:
            moterLeft = 0b1101;
            break;

        case 2:
            moterLeft = 0b1100;
            break;

        case 3:
            moterLeft = 0b1110;
            break;

        case 4:
            moterLeft = 0b0110;
            break;
        
        case 5:
            moterLeft = 0b0111;
            break;

        case 6:
            moterLeft = 0b0011;
            break;

        case 7:
            moterLeft = 0b1011;
            break;

    }
}


void moter_23_StraightRight(){
    if(moter.directionRight == FORWARD){
        if(countRight == 7){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(moter.directionRight == BACK){
        if(countRight == 0){
            countRight = 7;
        }else{
            countRight--;
        }
    }  
    switch(countRight){
        case 0:
            moterRight = 0b1011;
            break;

        case 1:
            moterRight = 0b0011;
            break;

        case 2:
            moterRight = 0b0111;
            break;

        case 3:
            moterRight = 0b0110;
            break;

        case 4:
            moterRight = 0b1110;
            break;
        
        case 5:
            moterRight = 0b1100;
            break;

        case 6:
            moterRight = 0b1101;
            break;

        case 7:
            moterRight = 0b1001;
            break;

    }
}

void debug(){
    printf("direction:");
    switch(data.direction){
        case FORWARD:
            printf("FORWARD");
            break;

        case STOP:
            printf("STOP");
            break;

        case RIGHT:
            printf("RIGHT");
            break;

        case LEFT:
            printf("LEFT");
            break;

        case TURN_LEFT:
            printf("TURN_LEFT");
            break;

        case TURN_RIGHT:
            printf("TURN_RIGHT");
            break;
    }
    printf("\t\t");
    printf("MODE:");
    switch(data.mode){
        case RUN_MODE_12:
            printf("RUN_MODE_12");
            break;
        
        case RUN_MODE_2:
            printf("RUN_MODE_2");
            break;
        
        case TEST_MODE:
            printf("TEST_MODE");
            break;
    }
    printf("\t");
    printf("volume:");
    printf("%1.2f",data.valueOfVolume1);
    printf("\t");
    printf("beforeWayStatus:");
    switch(data.beforeWayStatus){
        case ON_WAY:
            printf("ON_WAY");
            break;
        case BEFORE_WAY_IS_ALL_BLACK:
            printf("BEFORE_WAY_IS_ALL_BLACK");
            break;
        case MISSING:
            printf("MISSING");
            break;
    }
    printf("\t");
    printf("controlLeftVal:%1.2f, controlRightVal:%1.2f, ",data.controlLeftVal,data.controlRightVal);
    printf("p:%1.2f, i:%1.2f, d:%1.2f",rightVal.p,rightVal.i,rightVal.d);
    printf("\n");
}

void init(){
    data.flag = 1;
    digitalSwitch.mode(PullUp);
    modeSet();
    switch(data.mode){
        case RUN_MODE_3:
            value.speedLow = 0.04;
            value.speedMedium = 0.006;
            value.speedHigh = 0.003;
            value.DEBUG = false;
            break;
        case RUN_MODE_2:
            value.speedLow = 0.04;
            value.speedMedium = 0.006;
            value.speedHigh = 0.003;
            value.DEBUG = false;
            break;
        case RUN_MODE_23:
            value.speedLow = 0.02;
            value.speedMedium = 0.0040;
            value.speedHigh = 0.0015;
            value.DEBUG = false;
            break;
        case RUN_MODE_12:
            value.speedLow = 0.02;
            value.speedMedium = 0.0040;
            value.speedHigh = 0.0015;
            value.DEBUG = false;
            break;
        case TEST_MODE:
            value.speedLow = 0.1;
            value.speedMedium = 0.03;
            value.speedHigh = 0.015;
            value.DEBUG = true;
            break;
    }
    value.KP = 0.5;
    value.KI = 0.0;
    value.KD = 0.02;
}


void modeSet(){
    switch(digitalSwitch.read()){
        case 0x00:
            data.mode = RUN_MODE_12;
            break;
        case 0x01:
            data.mode = RUN_MODE_2;
            break;
        case 0x02:
            data.mode = TEST_MODE;
            break;
        case 0x03:
            data.mode = RUN_MODE_3;
            break;
        case 0x04:
            data.mode = RUN_MODE_23;
            break;
    }
}

void setValue(){
    value.Threshold = data.valueOfVolume1;
    value.KP = data.valueOfVolume1;
    value.KI = data.valueOfVolume2;
    value.KD = data.valueOfVolume3;

}
int main() {
    init();
    while(1) {
        getData();
        start();
        if(value.DEBUG) debug();
    }
}