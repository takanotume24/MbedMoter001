#include <mbed.h>
#define RUN_MODE 1
#define TEST_MODE 0
#if(RUN_MODE)
#define SPEED_LOW 0.04
#define SPEED_MEDIUM 0.008
#define SPEED_HIGH 0.004
#endif
#if(TEST_MODE)
#define SPEED_LOW 0.1
#define SPEED_MEDIUM 0.03
#define SPEED_HIGH 0.015
#endif

#define FORWARD 100
#define BACK 101
#define RIGHT 102
#define LEFT 103
#define STOP 104
#define TURN_RIGHT 105
#define TURN_LEFT 106

#define ON_WHITE 200
#define ON_BLACK 201

#define DEBUG 0
#define HIGH 1
#define LOW 0

Ticker moterTimerRight;
Ticker moterTimerLeft;
Ticker sensorTimer;
BusOut moterRight(PB_15,PB_13,PB_14,PC_4);
BusOut moterLeft(PC_12,PB_7,PA_15,PC_13);
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
}DATA;

typedef struct{
    int directionRight;
    int directionLeft;
    float speedRight;
    float speedLeft;
}MOTER;

DATA data;
MOTER moter;



void moter_1_StraightLeft();
void moter_1_StraightRight();
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

void setDirection(){
    data.oldDirection = data.direction;
    if(data.brightnessCenter == ON_BLACK){
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK ){
            data.newDirection = FORWARD;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
            data.newDirection = FORWARD;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
        }
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
            data.newDirection = LEFT;
            data.beforeDirection = LEFT;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
            data.newDirection = RIGHT;
            data.beforeDirection = RIGHT;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
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
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
        }
        if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
            data.beforeDirection = LEFT;
            data.newDirection = TURN_LEFT;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
        }
        if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
            data.beforeDirection = RIGHT;
            data.newDirection = TURN_RIGHT;
            moter.speedLeft = SPEED_HIGH;
            moter.speedRight = SPEED_HIGH;
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
    moterTimerLeft.attach(&moter_1_StraightLeft, moter.speedLeft);
}
void startMoterRight(){
    moterTimerRight.attach(&moter_1_StraightRight, moter.speedRight);
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
    moter.directionLeft = BACK;
    moter.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}
void goLeft(){
    moter.directionLeft = FORWARD;
    moter.directionRight = BACK;
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


void moter_1_StraightLeft(){

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

void moter_1_StraightRight(){
    // for(int i = 0.1; i < 0.01 ; i=i-0.01){
    //     moter.speedRight = i;
    //     moter_1_StraightRight();
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
    printf("\n");
}


int main() {
    data.flag = 1;
    while(1) {
        getData();
        start();
        if(DEBUG) debug();
    }
}