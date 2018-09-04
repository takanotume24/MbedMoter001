#include <mbed.h>
#define RUN_MODE_12     400
#define RUN_MODE_2      401
#define TEST_MODE       402

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

#define DEBUG           0
#define HIGH            1
#define LOW             0
k
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

typedef struct{
    int flag;
    int newDirection;
    int oldDirection;
    int direction;
    float valueOfSensorLeft;
    float valueOfSensorCenter;
    float valueOfSensorRight;
    int brightnessLeft;
    int brightnessCenter;
    int brightnessRight;
    int beforeDirection;
    int mode;
}DATA;

typedef struct{
    float speedLow;
    float speedMedium;
    float speedHigh;
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


int countRight = 0;
int countLeft = 0;

void getData(){
    data.valueOfSensorLeft = SensorLeft.read();
    data.valueOfSensorCenter = SensorCenter.read();
    data.valueOfSensorRight = SensorRight.read();
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
    switch(data.direction){
        case TURN_RIGHT:
            if(data.valueOfSensorLeft < 0.1){
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh;
            }else{
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh * data.valueOfSensorLeft ; 
            }
            break;
        case RIGHT:
            if(data.valueOfSensorLeft < 0.1){
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh;
            }else{
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh * data.valueOfSensorCenter ; 
            }
            break;

        case TURN_LEFT:
            if(data.valueOfSensorRight < 0.1){
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh;
            }else{
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh * data.valueOfSensorRight ; 
            }
            break;
        case LEFT:
            if(data.valueOfSensorCenter < 0.1){
                moter.speedLeft = value.speedHigh;
                moter.speedRight = value.speedHigh;
            }else{
                moter.speedLeft = value.speedHigh * data.valueOfSensorCenter;
                moter.speedRight = value.speedHigh;
            }

    }
}
void setDirection(){
    data.oldDirection = data.direction;
    if(data.brightnessCenter == ON_BLACK){
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK ){
            data.newDirection = FORWARD;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
            data.newDirection = FORWARD;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
            data.newDirection = LEFT;
            data.beforeDirection = LEFT;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
            data.newDirection = RIGHT;
            data.beforeDirection = RIGHT;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
    }
    if(data.brightnessCenter == ON_WHITE){
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
            switch(data.beforeDirection){
                case LEFT:
                    data.newDirection = TURN_LEFT;
                    break;
                case RIGHT:
                    data.newDirection = TURN_RIGHT;
                    break;
            }
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
            data.beforeDirection = LEFT;
            data.newDirection = TURN_LEFT;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
            data.beforeDirection = RIGHT;
            data.newDirection = TURN_RIGHT;
            moter.speedLeft = value.speedHigh;
            moter.speedRight = value.speedHigh;
        }
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
    printf("\t");
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
    printf("\n");
}

void init(){
    data.flag = 1;
    digitalSwitch.mode(PullUp);
    modeSet();
    switch(data.mode){
        case RUN_MODE_2:
            value.speedLow = 0.04;
            value.speedMedium = 0.006;
            value.speedHigh = 0.003;
            break;

        case RUN_MODE_12:
            value.speedLow = 0.02;
            value.speedMedium = 0.0040;
            value.speedHigh = 0.002;
            break;

        case TEST_MODE:
            value.speedLow = 0.1;
            value.speedMedium = 0.03;
            value.speedHigh = 0.015;
            break;
    }
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
    }
}
int main() {
    init();
    while(1) {
        getData();
        start();
        if(DEBUG) debug();
    }
}