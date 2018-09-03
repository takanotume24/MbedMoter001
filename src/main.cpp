#include <mbed.h>

#define SPEED_LOW 0.04
#define SPEED_HIGH 0.0025

#define FORWARD 100
#define BACK 101
#define RIGHT 102
#define LEFT 103
#define STOP 104

#define ON_WHITE 200
#define ON_BLACK 201

#define DEBUG false

Ticker moterTimerRight;
Ticker moterTimerLeft;
Ticker sensorTimer;
BusOut moterRight(PB_15,PB_13,PB_14,PC_4);
BusOut moterLeft(PC_12,PB_7,PA_15,PC_13);
DigitalOut SA(PB_15);
DigitalOut SB(PB_13);
DigitalOut SA_(PB_14);
DigitalOut SB_(PC_4);
AnalogIn SensorLeft(PC_2);
AnalogIn SensorRight(PB_1);

typedef struct{
    int flag;
    int newDirection;
    int oldDirection;
    int direction;
    float valueOfSensorLeft;
    float valueOfSensorRight;
    int brightnessLeft;
    int brightnessRight;
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

int countRight = 0;
int countLeft = 0;

void getData(){
    data.valueOfSensorLeft = SensorLeft.read();
    data.valueOfSensorRight = SensorRight.read();
    setBrightness();
}

void setBrightness(){
    if(data.valueOfSensorLeft < 0.5){
        data.brightnessLeft = ON_WHITE;
    }else{
        data.brightnessLeft = ON_BLACK;
    }

    if(data.valueOfSensorRight < 0.5){
        data.brightnessRight = ON_WHITE;
    }else{
        data.brightnessRight = ON_BLACK;
    }

}

void setDirection(){
    data.oldDirection = data.direction;
    if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK){
        data.newDirection = FORWARD;
        moter.speedLeft = SPEED_LOW;
        moter.speedRight = SPEED_LOW;
    }
    if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
        data.newDirection = FORWARD;
        moter.speedLeft = SPEED_HIGH;
        moter.speedRight = SPEED_HIGH;
    }
    if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
        data.newDirection = RIGHT;
        moter.speedLeft = SPEED_HIGH;
        moter.speedRight = SPEED_HIGH;
    }
    if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
        data.newDirection = LEFT;
        moter.speedLeft = SPEED_HIGH;
        moter.speedRight = SPEED_HIGH;
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
            
            default:
                stop();
                break;
        }
    }
}

void startMoterLeft(){
    moterTimerLeft.attach(&moter_12_StraightLeft, moter.speedLeft);
}
void startMoterRight(){
    moterTimerRight.attach(&moter_12_StraightRight, moter.speedRight);
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