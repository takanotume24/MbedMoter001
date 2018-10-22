//タイマー使うべきはPID計算、それ以外はWaitのほうがいい。（じゃないとWaitよりでかい時間のモーター100Hz以下が回らん）
#include "mbed.h"
#include "rtos.h"
#include "mbed_events.h"


enum RUN_MODE{
	RUN_MODE_2,
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
	float pid;
	float speed;
	float wait;
	enum DIRECTION direction;
}VAL;

VAL rightVal;
VAL leftVal;
VAL val;

DATA data;
VALUE value;
MOTOR motor;


int countRight = 0;
int countLeft = 0;

void setBrightness();
float limit(float);
void setRightPid();
void setLeftPid();
void setPid();
float freqToSeconds(int);
void motor_2_StraightLeft();
void motor_2_StraightRight();
void modeSet();



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
    leftVal.integral = leftVal.integral + (leftVal.diff[1] + leftVal.diff[0]) / 2.0 * speed;

    leftVal.p = value.KP * leftVal.diff[1];
    leftVal.i = value.KI * leftVal.integral;
    leftVal.d = value.KD * (leftVal.diff[1] - leftVal.diff[0]) / speed;

    return limit(abs(leftVal.p + leftVal.i + leftVal.p));
}


float pidCalcRight(float sensorVal, float targetVal, float speed){
    rightVal.diff[0] = rightVal.diff[1];
    rightVal.diff[1] = sensorVal - targetVal;
    rightVal.integral = rightVal.integral + (rightVal.diff[1] + rightVal.diff[0]) / 2.0 * speed;

    rightVal.p = value.KP * rightVal.diff[1];
    rightVal.i = value.KI * rightVal.integral;
    rightVal.d = value.KD * (rightVal.diff[1] - rightVal.diff[0]) / speed;

    return limit(abs(rightVal.p + rightVal.i + rightVal.p));
}

float pidCalc(float sensorVal, float targetVal, float speed){
	val.diff[0] = val.diff[1];
	val.diff[1] = sensorVal - targetVal;
	val.integral = val.integral + (val.diff[1] + val.diff[0]) /2.0 * speed;

	val.p = value.KP * val.diff[1];
	val.i = value.KI * val.integral;
	val.d = value.KD * (val.diff[1] - val.diff[0]) / speed;
	return limit(abs(val.p + val.i + val.d));
}

void setPid(){
	leftVal.pid = pidCalcLeft((1.0-data.valueOfSensorCenter)*2, 0.0, 0.01);
	rightVal.pid = pidCalcRight(data.valueOfSensorCenter*2, 0.0, 0.01);
	printf("called");
}

void setRightPid(){
	leftVal.pid = pidCalcLeft((1.0-data.valueOfSensorCenter)*2, 0.0, 0.01);
}

void setLeftPid(){
	rightVal.pid = pidCalcRight(data.valueOfSensorCenter*2, 0.0, 0.01);
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
}

void setDirection(){
	data.direction = FORWARD;
	if(leftVal.wait > 0.1){
		leftVal.wait = 0.1;
		leftVal.direction = STOP;
	}
	if(rightVal.wait > 0.1){
		rightVal.wait = 0.1;
		rightVal.direction = STOP;
	}
}

void startLeft(){
	while(1){
		leftVal.wait = freqToSeconds(334 * leftVal.pid);
		//printf("%f",freqToSeconds(334 * leftVal.pid));
		motor_2_StraightLeft();
		wait(1);
	}
}

void startRight(){
	while(1){
		rightVal.wait = freqToSeconds(334 * rightVal.pid);
		motor_2_StraightRight();
		wait(1);
	}
}



void startMoterLeft(){
	motor_2_StraightLeft();
}

void startMoterRight(){
	motor_2_StraightRight();
}

void stopMoterLeft(){
}
void stopMoterRight(){
}

void goStaight(){
    leftVal.direction = FORWARD;
    rightVal.direction = FORWARD;
    startMoterLeft();
    startMoterRight();
}

void stop(){
    stopMoterLeft();
    stopMoterRight();
}


void motor_2_StraightLeft(){
	switch(leftVal.direction){
		case FORWARD:
			if(countLeft == 3){
				countLeft = 0;
			}else{
				countLeft++;
			}
			break;

		case BACK:
			if(countRight == 0){
				countLeft = 3;
			}else{
				countLeft--;
			}
			break;

		default:
			break;
	}

	switch(leftVal.direction){
		case STOP:
			motorLeft = 0b0000;
			break;

		default:
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


}

void motor_2_StraightRight(){
	switch(rightVal.direction){
		case FORWARD:
			if(countRight == 3){
				countRight = 0;
			}else{
				countRight++;
			}
			break;

		case BACK:
			if(countRight == 0){
				countRight = 3;
			}else{
				countRight--;
			}
			break;

		default:
			break;
	}

	switch(rightVal.direction){
		case STOP:
			motorRight = 0b0000;
			break;

		default:
			switch(countRight){
				case 0:
					motorRight = 0b1001;
					break;

				case 1:
					motorRight = 0b1100;
					break;

				case 2:
					motorRight = 0b0110;
					break;

				case 3:
					motorRight = 0b0011;
					break;

			}
	}
}

void debug(){
#if 0
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

		case BACK:
			printf("BACK");
			break;
    }
    printf("\t");
#endif
#if 0
    printf("MODE:");
    switch(data.mode){
        case RUN_MODE_2:
            printf("RUN_MODE_2");
            break;

        case TEST_MODE:
            printf("TEST_MODE");
            break;
    }
    printf("\t");
#endif
#if 0
    printf("speedHigh:");
    printf("%.4f",value.speedHigh);
    printf("\t");
	printf("speedLeft:");
	printf("%.4f ", leftVal.speed);
	printf("speedRight:");
	printf("%.4f ", rightVal.speed);
	printf("\t");
#endif
#if 1
	printf("right:wait=");
	printf("%.4f",rightVal.wait);
	printf("\t");
	printf("pid=%.4f",rightVal.pid);
	printf("\t");
#endif
#if 1
	printf("left:wait=");
	printf("%.4f",leftVal.wait);
	printf("\t");
	printf("pid=%.4f",leftVal.pid);
	printf("\t");
#endif
#if 1
	printf("data.valueOfSensorCenter:");
	printf("%.4f",data.valueOfSensorCenter);
    printf("\n");
}
#endif

float freqToSeconds(int freq){
	return 1.0 / freq ;
}

void init(){
    digitalSwitch.mode(PullUp);
    modeSet();
    switch(data.mode){
        case RUN_MODE_2:
            value.speedLow = freqToSeconds(0);
            value.speedMedium = freqToSeconds(167);
            value.speedHigh = freqToSeconds(334);
            value.DEBUG = true;
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
        case 0x01:
            data.mode = RUN_MODE_2;
            break;
        case 0x02:
            data.mode = TEST_MODE;
            break;
		default:
			printf("mode not selected!");
			break;
    }
}


int main() {
	EventQueue queue;
	Thread motorLeftThread;
	Thread motorRightThread;
	motorLeftThread.start(startLeft);
	motorRightThread.start(startRight);
	queue.call_every(100,&setPid);
	init();
	queue.dispatch();
    while(1) {
		setDirection();
        getData();
        if(value.DEBUG) debug();
		value.KP = volume * 2;
    }

}
