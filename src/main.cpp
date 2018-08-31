#include <mbed.h>
#define SPEED 0.01
#define FORWARD 100
#define BACK 101
#define RIGHT 102
#define LEFT 103
#define STOP 104

#define ON_WHITE 200
#define ON_BLACK 201

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
}MOTER;

DATA data;
MOTER moter;



void moterStraightLeft();
void moterStraightRight();
void getData();
void setDirection();
void start();
void goLeft();
void goRight();
void goStaight();
void stop();
void setBrightness();

int countRight = 0;
int countLeft = 0;

void getData(){
    data.valueOfSensorLeft = SensorLeft.read();
    data.valueOfSensorRight = SensorRight.read();
    setBrightness();
}

void setBrightness(){
    if(data.valueOfSensorLeft < 0.5){
        data.brightnessLeft = ON_BLACK;
    }else{
        data.brightnessLeft = ON_WHITE;
    }

    if(data.valueOfSensorRight < 0.5){
        data.brightnessRight = ON_BLACK;
    }else{
        data.brightnessRight = ON_WHITE;
    }

}

void setDirection(){
    data.oldDirection = data.direction;
    if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK){
        data.newDirection = FORWARD;
    }
    if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
        data.newDirection = STOP;
    }
    if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
        data.newDirection = RIGHT;
    }
    if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
        data.newDirection = LEFT;
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
    moterTimerLeft.attach(&moterStraightLeft, SPEED);
}
void startMoterRight(){
    moterTimerRight.attach(&moterStraightRight, SPEED);
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


void moterStraightLeft(){
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

void moterStraightRight(){
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

void moter12(){
    switch(countRight){
        case 0:
            SA = 1;
            SB = 0;
            SA_ = 0;
            SB_ = 0;
            countRight++;
            break;

        case 1:
            SA = 1;
            SB = 1;
            SA_ = 0;
            SB_ = 0;
            countRight++;
            break;

        case 2:
            SA = 0;
            SB = 1;
            SA_ = 0;
            SB_ = 0;
            countRight++;
            break;

        case 3:
            SA = 0;
            SB = 1;
            SA_ = 1;
            SB_ = 0;
            countRight++;
            break;

        case 4:
            SA = 0;
            SB = 0;
            SA_ = 1;
            SB_ = 0;
            countRight++;
            break;
        
        case 5:
            SA = 0;
            SB = 0;
            SA_ = 1;
            SB_ = 1;
            countRight++;
            break;

        case 6:
            SA = 0;
            SB = 0;
            SA_ = 0;
            SB_ = 1;
            countRight++;
            break;

        case 7:
            SA = 1;
            SB = 0;
            SA_ = 0;
            SB_ = 1;
            countRight = 0;
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
        debug();
    }
}