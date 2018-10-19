//タイマー使うべきはPID計算、それ以外はWaitのほうがいい。（じゃないとWaitよりでかい時間のモーター100Hz以下が回らん）
#include <mbed.h>
enum RUN_MODE{
	RUN_MODE_2,
	RUN_MODE_3,
	RUN_MODE_12,
	RUN_MODE_23,
	TEST_MODE
};

enum BRIGHTNESS{
	ON_WHITE,
	ON_BLACK
};

enum WAY_STATUS{
	ON_STRAIGHT_WAY,
	ON_CURVE_WAY
};

enum ON_WAY{
	ON_WAY,
	MISSING,
	ALL_WAY,
	BEFORE_WAY_IS_ALL_BLACK
};

enum DIRECTION{
	FORWARD,
	BACK,
	RIGHT,
	LEFT,
	STOP,
	TURN_RIGHT,
	TURN_LEFT
};

#define HIGH            1
#define LOW             0

int histry[10];
Ticker motorTimerRight;
Ticker motorTimerLeft;
Ticker sensorTimer;
BusOut motorRight(PB_15,PB_13,PB_14,PC_4);
BusOut motorLeft(PC_12,PB_7,PA_15,PC_13);

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
AnalogIn volume(PC_5);
Serial pc(USBTX, USBRX, 115200);


typedef struct{
    int flag;
    enum DIRECTION oldDirection;
	enum DIRECTION newDirection;
    enum DIRECTION direction;
    float valueOfSensorLeft;
    float valueOfSensorCenter;
    float valueOfSensorRight;
    float valueOfVolume;
    enum BRIGHTNESS brightnessLeft;
    enum BRIGHTNESS brightnessCenter;
    enum BRIGHTNESS brightnessRight;
    enum DIRECTION beforeDirection;
    enum RUN_MODE mode;
    enum ON_WAY onWay;
    enum ON_WAY beforeWayStatus;
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
    enum DIRECTION directionRight;
    enum DIRECTION directionLeft;
    float speedRight;
    float speedLeft;
}MOTOR;

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

RIGHT_VAL rightVal;
LEFT_VAL leftVal;

DATA data;
VALUE value;
MOTOR motor;



void motor_2_StraightLeft();
void motor_2_StraightRight();
void getData();
void setDirection();
void start();
void goLeft();
void goRight();
void goStaight();
void stop();
void setBrightness();
void motor_12_StraightRight();
void motor_12_StraightLeft();
void turnLeft();
void turnRight();
void modeSet();
void motor_3_StraightLeft();
void motor_3_StraightRight();
void motor_23_StraightLeft();
void motor_23_StraightRight();
float limit(float);
float freqToSeconds(int);
int countRight = 0;
int countLeft = 0;

void getData(){
    data.valueOfSensorLeft = SensorLeft.read();
    data.valueOfSensorCenter = SensorCenter.read();
    data.valueOfSensorRight = SensorRight.read();
    data.valueOfVolume = volume.read();
    setBrightness();
}

void setBrightness(){
    if(data.valueOfSensorLeft < 0.5){
        data.brightnessLeft = ON_BLACK;
        ledLeft = HIGH;
    }else{
        data.brightnessLeft = ON_WHITE;
        ledLeft = LOW;
    }

    if(data.valueOfSensorRight < 0.5){
        data.brightnessRight = ON_BLACK;
        ledRight = HIGH;
    }else{
        data.brightnessRight = ON_WHITE;
        ledRight = LOW;
    }

    if(data.valueOfSensorCenter < 0.5){
        data.brightnessCenter = ON_BLACK;
        ledCenter = HIGH;
    }else{
        data.brightnessCenter = ON_WHITE;
        ledCenter = LOW;
    }

}


float pidCalcLeft(float sensorVal, float targetVal, float speed){
    leftVal.diff[0] = leftVal.diff[1];
    leftVal.diff[1] = sensorVal - targetVal;
    leftVal.integral = leftVal.integral +(leftVal.diff[1]+leftVal.diff[0])/2.0*speed;

    leftVal.p = value.KP * leftVal.diff[1];
    leftVal.i = value.KI * leftVal.integral;
    leftVal.d = value.KD * (leftVal.diff[1]-leftVal.diff[0])/speed;

    return limit(abs(leftVal.p+leftVal.i+leftVal.p));
}


float pidCalcRight(float sensorVal, float targetVal, float speed){
    rightVal.diff[0] = rightVal.diff[1];
    rightVal.diff[1] = sensorVal - targetVal;
    rightVal.integral = rightVal.integral +(rightVal.diff[1]+rightVal.diff[0])/2.0*speed;

    rightVal.p = value.KP * rightVal.diff[1];
    rightVal.i = value.KI * rightVal.integral;
    rightVal.d = value.KD * (rightVal.diff[1]-rightVal.diff[0])/speed;

    return limit(abs(rightVal.p+rightVal.i+rightVal.p));
}

float limit(float val){
    if(val > 1){
        return 1.0;
    }
    else if(val < 0){
        return 0.0;
    }
	else{
		return val;
	}
    return 0;
}


void setSpeed(){
    switch(data.direction){
        case FORWARD:
            //motor.speedLeft = value.speedHigh / pidCalcLeft(data.valueOfSensorLeft, 0.0, 0.001);
			motor.speedLeft = freqToSeconds(334 * pidCalcLeft((1.0-data.valueOfSensorCenter)*2, 0.0, 0.01));
            motor.speedRight = freqToSeconds(334 * pidCalcLeft(data.valueOfSensorCenter*2, 0.0, 0.01));
            break;
		default :
			break;
        // case TURN_RIGHT:
        //     switch(data.onWay){
        //         case ON_WAY:
        //             if(data.valueOfSensorLeft < value.Threshold){
        //                 motor.speedLeft = value.speedHigh;
        //                 motor.speedRight = value.speedHigh;
        //             }else{
        //                 motor.speedLeft = value.speedHigh ;
        //                 motor.speedRight = value.speedHigh * (1+data.valueOfSensorLeft) ;
        //             }
        //             break;
		//
        //         case MISSING:
        //             motor.speedLeft = value.speedHigh;
        //             motor.speedRight = value.speedHigh;
        //     }
        //     break;
		//
        // case RIGHT:
        //     if(data.valueOfSensorCenter < value.Threshold){
        //         motor.speedLeft = value.speedHigh;
        //         motor.speedRight = value.speedHigh;
        //     }else{
        //         motor.speedLeft = value.speedHigh;
        //         motor.speedRight = value.speedHigh  * (1+data.valueOfSensorCenter) ;
        //     }
        //     break;
		//
        // case TURN_LEFT:
        //     switch(data.onWay){
        //         case ON_WAY:
        //             if(data.valueOfSensorRight < value.Threshold){
        //                 motor.speedLeft = value.speedHigh;
        //                 motor.speedRight = value.speedHigh;
        //             }else{
        //                 motor.speedLeft = value.speedHigh * (1+data.valueOfSensorRight);
        //                 motor.speedRight = value.speedHigh ;
        //             }
        //             break;
		//
        //         case MISSING:
        //             motor.speedLeft = value.speedHigh;
        //             motor.speedLeft = value.speedHigh;
        //     }
        //     break;
		//
        // case LEFT:
        //     if(data.valueOfSensorCenter < value.Threshold){
        //         motor.speedLeft = value.speedHigh;
        //         motor.speedRight = value.speedHigh;
        //     }else{
        //         motor.speedLeft = value.speedHigh * (1+data.valueOfSensorCenter);
        //         motor.speedRight = value.speedHigh;
        //     }
		// 	break;
		//
		// case STOP:
		// 	break;

    }
}
//  void setDirection(){
//     data.oldDirection = data.direction;
//     switch(data.brightnessCenter){
//         case ON_BLACK:  //真ん中が線上
//             data.onWay = ON_WAY;
//             if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK ){
//                 data.newDirection = FORWARD;
//                 data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
//             }
//             if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
//                 data.newDirection = FORWARD;
//                 data.beforeWayStatus = ON_WAY;
//             }
//             if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
//                 switch(data.beforeWayStatus){
//                     case BEFORE_WAY_IS_ALL_BLACK:
//                         data.newDirection = RIGHT;
//                         data.beforeDirection = RIGHT;
//                         data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
//                         break;
//
//                     case ON_WAY:
//                         data.newDirection = LEFT;
//                         data.beforeDirection = LEFT;
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//                 }
//             }
//             if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
//                 switch(data.beforeWayStatus){
//                     case BEFORE_WAY_IS_ALL_BLACK:
//                         data.newDirection = LEFT;
//                         data.beforeDirection = LEFT;
//                         data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
//                         break;
//
//                     case ON_WAY:
//                         data.newDirection = RIGHT;
//                         data.beforeDirection = RIGHT;
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//                 }
//             }
//             break;
//
//          case ON_WHITE:  //真ん中が線上にない
//             if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_WHITE){
//                 data.onWay = MISSING;
//                 switch(data.beforeWayStatus){
//                     case BEFORE_WAY_IS_ALL_BLACK:
//                         switch(data.beforeDirection){
//                             case LEFT:
//                                 data.newDirection = TURN_RIGHT;
//                                 break;
//
//                             case RIGHT:
//                                 data.newDirection = TURN_LEFT;
//                                 break;
//                         }
//                         data.beforeWayStatus = BEFORE_WAY_IS_ALL_BLACK;
//                         break;
//
//                     case ON_WAY:
//                         switch(data.direction){
//                             case LEFT:
//                                 data.newDirection = TURN_LEFT;
//                                 break;
//
//                             case RIGHT:
//                                 data.newDirection = TURN_RIGHT;
//                                 break;
//                         }
//                         data.beforeWayStatus = MISSING;
//                         break;
//
//
//                     case MISSING:
//                         switch(data.direction){
//                             case LEFT:
//                                 data.newDirection = TURN_LEFT;
//                                 break;
//
//                             case RIGHT:
//                                 data.newDirection = TURN_RIGHT;
//                                 break;
//                         }
//                         data.beforeWayStatus = MISSING;
//                         break;
//                 }
//             }
//             if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_WHITE){
//                 data.onWay = ON_WAY;
//                 switch(data.beforeWayStatus){
//                     case BEFORE_WAY_IS_ALL_BLACK:
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//
//                     case ON_WAY:
//                     case MISSING:
//                         data.beforeDirection = LEFT;
//                         data.newDirection = LEFT;
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//                 }
//             }
//             if(data.brightnessLeft == ON_WHITE && data.brightnessRight == ON_BLACK){
//                 data.onWay = ON_WAY;
//                 switch(data.beforeWayStatus){
//                     case BEFORE_WAY_IS_ALL_BLACK:
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//
//                     case ON_WAY:
//                     case MISSING:
//                         data.beforeDirection = RIGHT;
//                         data.newDirection = RIGHT;
//                         data.beforeWayStatus = ON_WAY;
//                         break;
//                 }
//             }
//             if(data.brightnessLeft == ON_BLACK && data.brightnessRight == ON_BLACK){
//                 data.onWay = ON_WAY;
//                 data.newDirection = FORWARD;
//                 data.beforeWayStatus = ON_WAY;
//             }
//             break;
//
//     }
//     if(data.brightnessCenter == ON_BLACK){
//
//     }
//     if(data.brightnessCenter == ON_WHITE){
//
//     }
//
//     if(data.newDirection == data.oldDirection){
//         data.flag = 0;
//     }else{
//         data.direction = data.newDirection;
//         data.flag = 1;
//     }
// }
void setDirection(){
	data.direction = FORWARD;
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
            motorTimerLeft.attach(&motor_2_StraightLeft, motor.speedLeft);
            break;
        case RUN_MODE_12:
            motorTimerLeft.attach(&motor_12_StraightLeft, motor.speedLeft);
            break;
        case RUN_MODE_2:
            motorTimerLeft.attach(&motor_2_StraightLeft, motor.speedLeft);
            break;
        case RUN_MODE_3:
            motorTimerLeft.attach(&motor_3_StraightLeft, motor.speedLeft);
            break;
        case RUN_MODE_23:
            motorTimerLeft.attach(&motor_23_StraightLeft, motor.speedLeft);
            break;
    }
}
void startMoterRight(){
    switch(data.mode){
        case TEST_MODE :
            motorTimerRight.attach(&motor_2_StraightRight, motor.speedRight);
            break;
        case RUN_MODE_12:
            motorTimerRight.attach(&motor_12_StraightRight, motor.speedRight);
            break;
        case RUN_MODE_2:
            motorTimerRight.attach(&motor_2_StraightRight, motor.speedRight);
            break;
        case RUN_MODE_3:
            motorTimerRight.attach(&motor_3_StraightRight, motor.speedRight);
            break;
        case RUN_MODE_23:
            motorTimerRight.attach(&motor_23_StraightRight, motor.speedRight);
    }
    motorTimerRight.attach(&motor_2_StraightRight, motor.speedRight);
}

void stopMoterLeft(){
    motorTimerLeft.detach();
}
void stopMoterRight(){
    motorTimerRight.detach();
}

void goStaight(){
    motor.directionLeft = FORWARD;
    motor.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}

void goRight(){
    motor.directionLeft = FORWARD;
    motor.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}
void goLeft(){
    motor.directionLeft = FORWARD;
    motor.directionRight = FORWARD;
    startMoterLeft();
    startMoterRight();
}
void turnRight(){
    motor.directionLeft = FORWARD;
    motor.directionRight = FORWARD;
    startMoterLeft();
    stopMoterRight();
}
void turnLeft(){
    motor.directionLeft = FORWARD;
    motor.directionRight = FORWARD;
    stopMoterLeft();
    startMoterRight();
}
void stop(){
    stopMoterLeft();
    stopMoterRight();
}


void motor_2_StraightLeft(){

    if(motor.directionLeft== FORWARD){
        if(countLeft == 3){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(motor.directionLeft == BACK){
        if(countRight == 0){
            countLeft = 3;
        }else{
            countLeft--;
        }
    }

    switch(countLeft){
        case 0:
            motorLeft = 0b1001;
            break;

        case 1:
            motorLeft = 0b1100;
            break;

        case 2:
            motorLeft = 0b0110;
            break;

        case 3:
            motorLeft = 0b0011;
            break;

    }

}

void motor_2_StraightRight(){
    if(motor.directionRight == FORWARD){
        if(countRight == 3){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(motor.directionRight == BACK){
        if(countRight == 0){
            countRight = 3;
        }else{
            countRight--;
        }
    }
    switch(countRight){
        case 0:
            motorRight = 0b0011;
            break;

        case 1:
            motorRight = 0b0110;
            break;

        case 2:
            motorRight = 0b1100;
            break;

        case 3:
            motorRight = 0b1001;
            break;

        default :
            break;
    }
}
void motor_3_StraightLeft(){

    if(motor.directionLeft== FORWARD){
        if(countLeft == 3){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(motor.directionLeft == BACK){
        if(countRight == 0){
            countLeft = 3;
        }else{
            countLeft--;
        }
    }

    switch(countLeft){
        case 0:
            motorLeft = 0b0111;
            break;

        case 1:
            motorLeft = 0b1011;
            break;

        case 2:
            motorLeft = 0b1101;
            break;

        case 3:
            motorLeft = 0b1110;
            break;

    }

}

void motor_3_StraightRight(){
    if(motor.directionRight == FORWARD){
        if(countRight == 3){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(motor.directionRight == BACK){
        if(countRight == 0){
            countRight = 3;
        }else{
            countRight--;
        }
    }
    switch(countRight){
        case 0:
            motorRight = 0b1110;
            break;

        case 1:
            motorRight = 0b1101;
            break;

        case 2:
            motorRight = 0b1011;
            break;

        case 3:
            motorRight = 0b0111;
            break;

        default :
            break;
    }
}
void motor_12_StraightLeft(){
    if(motor.directionLeft == FORWARD){
        if(countLeft == 7){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(motor.directionLeft == BACK){
        if(countLeft == 0){
            countLeft = 7;
        }else{
            countLeft--;
        }
    }
    switch(countLeft){
        case 0:
            motorLeft = 0b1001;
            break;

        case 1:
            motorLeft = 0b1000;
            break;

        case 2:
            motorLeft = 0b1100;
            break;

        case 3:
            motorLeft = 0b0100;
            break;

        case 4:
            motorLeft = 0b0110;
            break;

        case 5:
            motorLeft = 0b0010;
            break;

        case 6:
            motorLeft = 0b0011;
            break;

        case 7:
            motorLeft = 0b0001;
            break;

    }
}


void motor_12_StraightRight(){
    if(motor.directionRight == FORWARD){
        if(countRight == 7){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(motor.directionRight == BACK){
        if(countRight == 0){
            countRight = 7;
        }else{
            countRight--;
        }
    }
    switch(countRight){
        case 0:
            motorRight = 0b0001;
            break;

        case 1:
            motorRight = 0b0011;
            break;

        case 2:
            motorRight = 0b0010;
            break;

        case 3:
            motorRight = 0b0110;
            break;

        case 4:
            motorRight = 0b0100;
            break;

        case 5:
            motorRight = 0b1100;
            break;

        case 6:
            motorRight = 0b1000;
            break;

        case 7:
            motorRight = 0b1001;
            break;

    }
}
void motor_23_StraightLeft(){
    if(motor.directionLeft == FORWARD){
        if(countLeft == 7){
            countLeft = 0;
        }else{
            countLeft++;
        }
    }
    if(motor.directionLeft == BACK){
        if(countLeft == 0){
            countLeft = 7;
        }else{
            countLeft--;
        }
    }
    switch(countLeft){
        case 0:
            motorLeft = 0b1001;
            break;

        case 1:
            motorLeft = 0b1101;
            break;

        case 2:
            motorLeft = 0b1100;
            break;

        case 3:
            motorLeft = 0b1110;
            break;

        case 4:
            motorLeft = 0b0110;
            break;

        case 5:
            motorLeft = 0b0111;
            break;

        case 6:
            motorLeft = 0b0011;
            break;

        case 7:
            motorLeft = 0b1011;
            break;

    }
}


void motor_23_StraightRight(){
    if(motor.directionRight == FORWARD){
        if(countRight == 7){
            countRight = 0;
        }else{
            countRight++;
        }
    }
    if(motor.directionRight == BACK){
        if(countRight == 0){
            countRight = 7;
        }else{
            countRight--;
        }
    }
    switch(countRight){
        case 0:
            motorRight = 0b1011;
            break;

        case 1:
            motorRight = 0b0011;
            break;

        case 2:
            motorRight = 0b0111;
            break;

        case 3:
            motorRight = 0b0110;
            break;

        case 4:
            motorRight = 0b1110;
            break;

        case 5:
            motorRight = 0b1100;
            break;

        case 6:
            motorRight = 0b1101;
            break;

        case 7:
            motorRight = 0b1001;
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
    printf("speedHigh:");
    printf("%.4f",value.speedHigh);
    printf("\t");
	printf("speedLeft:");
	printf("%.4f ", motor.speedLeft);
	printf("speedRight:");
	printf("%.4f ", motor.speedRight);
	printf("\t");
    // printf("beforeWayStatus:");
    // switch(data.beforeWayStatus){
    //     case ON_WAY:
    //         printf("ON_WAY");
    //         break;
    //     case BEFORE_WAY_IS_ALL_BLACK:
    //         printf("BEFORE_WAY_IS_ALL_BLACK");
    //         break;
    //     case MISSING:
    //         printf("MISSING");
    //         break;
    // }
    // printf("\t");
    printf("\n");
}

float freqToSeconds(int freq){
	return 1.0 / freq ;
}

void init(){
    data.flag = 1;
    digitalSwitch.mode(PullUp);
    modeSet();
    switch(data.mode){
        case RUN_MODE_3:
            value.speedLow = freqToSeconds(25);
            value.speedMedium = freqToSeconds(167);
            value.speedHigh = freqToSeconds(334);
            value.DEBUG = false;
            break;
        case RUN_MODE_2:
            value.speedLow = freqToSeconds(0);
            value.speedMedium = freqToSeconds(167);
            value.speedHigh = freqToSeconds(334);
            value.DEBUG = true;
            break;
        case RUN_MODE_23:
            value.speedLow = freqToSeconds(50);
            value.speedMedium = freqToSeconds(250);
            value.speedHigh = freqToSeconds(667);
            value.DEBUG = false;
            break;
        case RUN_MODE_12:
            value.speedLow = freqToSeconds(50);
            value.speedMedium =	freqToSeconds(250);
            value.speedHigh = freqToSeconds(667);
            value.DEBUG = false;
            break;
        case TEST_MODE:
            value.speedLow = freqToSeconds(10);
            value.speedMedium = freqToSeconds(33);
            value.speedHigh = freqToSeconds(66);
            value.DEBUG = true;
            break;
    }
    value.Threshold = data.valueOfVolume;
	value.KP = 0.804;
	value.KI = 0;
	value.KD = 0;
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
int main() {
    init();
    while(1) {
        getData();
        start();
        if(value.DEBUG) debug();
		wait_ms (10);
		value.KP = volume*2;
    }
}
