/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *          Geunsun Lee <gs86.lee@samsung.com>
 *          Eunyoung Lee <ey928.lee@samsung.com>
 *          Junkyu Han <junkyu.han@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RCC_RESOURCE_PWM_MOTOR_H__
#define __RCC_RESOURCE_PWM_MOTOR_H__
void resource_close_pwm_motor(void);
int resource_set_pwm_motor_value(double duty_cycle_ms);
#endif

#include <unistd.h>
#include <glib.h>
#include <Ecore.h>
#include <tizen.h>
#include <service_app.h>

#include "log.h"
#include "resource.h"
#include "connectivity.h"
#include "controller.h"
#include "controller_util.h"
#include "webutil.h"

typedef struct app_data_s {
	Ecore_Timer *getter_timer;
	connectivity_resource_s *resource_info;
} app_data;

#define CONNECTIVITY_KEY "opened"

/*		 Timer_Duration		*/
#define DISTANCE_DURATION (1.0f)
#define TEMPERATURE_DURATION (60.0f)
#define ILLUMINANCE_DURATION (60.0f)
#define TILT_DURATION (1.0f)

/*		Sensor_Pin_Number		*/
#define FRONT_US_TRIG 21
#define FRONT_US_ECHO 20
#define BACK_US_TRIG 19
#define BCAK_US_ECHO 26
#define TILT_PIN 25

/*						timer 						*/
static Ecore_Timer *front_distance_timer = NULL;
static Ecore_Timer *back_distance_timer = NULL;
static Ecore_Timer *temperature_timer = NULL;
static Ecore_Timer *illuminanace_timer = NULL;
static Ecore_Timer *tilt_timer = NULL;

/*--------------------- Ultrasonic func -----------------------*/
Eina_Bool _front_distance_timer_cb(void *data)
{
	int ret = 0;

	ret = resource_read_ultrasonic_sensor(FRONT_US_TRIG, FRONT_US_ECHO, __front_distance_cb, NULL);
	retv_if(ret < 0, ECORE_CALLBACK_CANCEL);

	return ECORE_CALLBACK_RENEW;
}
void __front_distance_cb(double dist, void *data)
{
	_D("front distance : %f ",dist);
	if(dist < 0 || dist > 300){
		_D("ultrasonic impossible to measure..");
	}
	else if(dist >0 && dist <50){
		_D("who is nearby.. "); //앞에 무엇인가 있으면 멈춤
		resource_set_pwm_motor_value(0); // motor off
	}

}
Eina_Bool _back_distance_timer_cb(void *data)
{
	int ret = 0;

	ret = resource_read_ultrasonic_sensor(BACK_US_TRIG, BCAK_US_ECHO, __back_distance_cb, NULL);
	retv_if(ret < 0, ECORE_CALLBACK_CANCEL);

	return ECORE_CALLBACK_RENEW;
}
void __back_distance_cb(double dist, void *data)
{
	_D("back distance : %f ",dist);
	if(dist < 0 || dist > 300){
			_D("ultrasonic impossible to measure..");
		}
		else if(dist >0 && dist <30){
			_D("keep away form user..");
			resource_set_pwm_motor_value(10); // motor on
		}
		else if(dist >= 30){
			_D("keep away form user..");
			resource_set_pwm_motor_value(0); // motor off
		}
}
/*-------------------------------------------------------------*/

/*-------------------- temperature func -----------------------*/
static Eina_Bool _temperature_timer_cb(void *data)
{
	int ret = 0;
	double target_object = 0.0f;
	double target_ambient = 0.0f;

	ret = resource_read_mcu90615(&temp_object, &target_ambient);
	retv_if(ret < 0, ECORE_CALLBACK_CANCEL);

	if(temp_object == 0.0f) return ECORE_CALLBACK_RENEW;
	target_object = temp_object;

	_D("Temperature : %f and %f", target_object, target_ambient);

	if( target_object > HIGH_TEMPERATURE){
		// ~~~..
	}

	return ECORE_CALLBACK_RENEW;
}
/*-------------------------------------------------------------*/

/*-------------------- illuminance func -----------------------*/
static Eina_Bool _illuminance_led_timer_cb(void *data){

	int ret;
	unit32_t value;

	ret = resource_read_illuminance_sensor(1, &value);
	retv_if(ret < 0, ECORE_CALLBACK_CANCEL);

	if(value < 1.0){
		_D("illuminance value : %f", value);
		resource_write_led(18, 1); // 18핀에 연결된 led 점등
	}

	return ECORE_CALLBACK_RENEW;
}
/*-------------------------------------------------------------*/

/*------------------------ tilt func --------------------------*/
static Eina_Bool _tilt_timer_cb(void *data){

	int ret;
	unit32_t out_value;

	ret = resource_read_tilt_sensor(TILT_PIN, &out_value);
	retv_if(ret < 0, ECORE_CALLBACK_CANCEL);

	if(out_value){
		//~~~..
	}

	return ECORE_CALLBACK_RENEW;
}
/*-------------------------------------------------------------*/

static bool service_app_create(void *data)
{
	_D("create");

	//reset~
	//~~..

	return true;
}
static void service_app_terminate(void *data)
{
	app_data *ad = (app_data *)data;

	_D("terminate");

	/*		 timer close 		*/
	if (front_distance_timer)
		ecore_timer_del(front_distance_timer);
	if (back_distance_timer)
		ecore_timer_del(back_distance_timer);
	if (temperature_timer)
			ecore_timer_del(back_distance_timer);
	if (illuminanace_timer)
			ecore_timer_del(back_distance_timer);
	if (tilt_timer)
			ecore_timer_del(back_distance_timer);

	resource_close_pwm_motor();
	resource_close_led(18);
	free(ad);

}
static void service_app_control(app_control_h app_control, void *data)
{
	_D("control");

	/*			timer add			*/
	front_distance_timer = ecore_timer_add(DISTANCE_DURATION, _front_distance_timer_cb, NULL);
	back_distance_timer = ecore_timer_add(DISTANCE_DURATION, _back_distance_timer_cb, NULL);
	temperature_timer = ecore_timer_add(TEMPERATURE_DURATION, _temperature_timer_cb, NULL);
	illuminanace_timer = ecore_timer_add(ILLUMINANCE_DURATION, _illuminance_led_timer_cb, NULL);
	tilt_timer = ecore_timer_add(TILT_DURATION, _tilt_timer_cb, NULL);

}

static void service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
}

static void service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
	app_data *ad = NULL;
	int ret = 0;
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	ret = service_app_main(argc, argv, &event_callback, ad);

	return ret;
}
