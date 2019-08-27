/*
 * Copyright (c) 2017 FH Dortmund and others
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Description:
 *     RoverQMC5883L API - Interfaces for Rover QMC5883L bearing sensor application development
 *     Header file
 *
 * Contributors:
 *    M.Ozcelikors <mozcelikors@gmail.com>, created RoverQMC5883L class 04.12.2017
 *
 */


#include "qmc5883l.h"

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

RoverQMC5883L::RoverQMC5883L()
:QMC5883L_ADDRESS(0x0D),			//default: 0x0D
 CALIBRATION_DURATION(10000), 	//compass calibration has a duration of 5 seconds
 DECLINATION_ANGLE(0.0413),  	//correction factor for location Paderborn
 calibration_start(0),
 ROVERQMC5883L_SETUP_(0),
 xMinRaw(0),
 xMaxRaw(0),
 yMinRaw(0),
 yMaxRaw(0),
 i2c_qmc588l_fd(-1)
{

}

RoverQMC5883L::~RoverQMC5883L(){}

void RoverQMC5883L::initialize (void)
{
#if !SIMULATOR
	if ((i2c_qmc588l_fd = wiringPiI2CSetup(this->QMC5883L_ADDRESS)) < 0)
	{
		std::cout << ("Failed to initialize QMC588L compass sensor") << std::endl;
	}

	if (i2c_qmc588l_fd >= 0)
	{
		wiringPiI2CWriteReg8 (i2c_qmc588l_fd, 0x0B, 0x01); //init SET/PERIOD register

		/*
		Define
		OSR = 512
		Full Scale Range = 8G(Gauss)
		ODR = 200HZ
		set continuous measurement mode
		*/
		wiringPiI2CWriteReg8 (i2c_qmc588l_fd, 0x09, 0x1D);
	}

	this->calibration_start = millis();

#endif
	this->ROVERQMC5883L_SETUP_ = 1;
	//
	// To do a software reset
	// wiringPiI2CWriteReg8 (i2c_hmc588l_fd, 0x0A, 0x80);
	//
}

float RoverQMC5883L::read (void)
{
#if SIMULATOR
	return 0.5;
#else
	if (this->ROVERQMC5883L_SETUP_ != 1)
	{
		std::cout << (stderr,"You havent set up RoverQMC5883L. Use RoverQMC5883L()::initialize() !\n") << std::endl;
	}
	else
	{
		int8_t buffer[6];

		//wiringPiI2CWrite (i2c_hmc588l_fd, 0x00); //Start with register 3
		//delay(25);

		buffer[0] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x00); //LSB x
		buffer[1] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x01); //MSB x
		buffer[2] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x02); //LSB y
		buffer[3] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x03); //MSB y
		buffer[4] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x04); //LSB z
		buffer[5] = wiringPiI2CReadReg8(i2c_qmc588l_fd, 0x05); //MSB z

		int xRaw = (((int) buffer[1] << 8) & 0xff00) | buffer[0];
		int yRaw = (((int) buffer[3] << 8) & 0xff00) | buffer[2];

		//if calibration is active calculate minimum and maximum x/y values for calibration
		if (millis() <= this->calibration_start + this->CALIBRATION_DURATION)
		{
			xMinRaw = MINIMUM_(xRaw, xMinRaw);

			xMaxRaw = MAXIMUM_(xRaw, xMaxRaw);

			yMinRaw = MINIMUM_(yRaw, yMinRaw);

			yMaxRaw = MAXIMUM_(yRaw, yMaxRaw);
		}

		//calibration: move and scale x coordinates based on minimum and maximum values to get a unit circle
		float xf = xRaw - (float) (xMinRaw + xMaxRaw) / 2.0f;
		xf = xf / (xMinRaw + xMaxRaw) * 2.0f;

		//calibration: move and scale y coordinates based on minimum and maximum values to get a unit circle
		float yf = yRaw - (float) (yMinRaw + yMaxRaw) / 2.0f;
		yf = yf / (yMinRaw + yMaxRaw) * 2.0f;

		float bearing = atan2(yf, xf);

		//location specific magnetic field correction
		bearing += this->DECLINATION_ANGLE;

		if (bearing < 0) {
			bearing += 2 * M_PI;
		}

		if (bearing > 2 * M_PI) {
			bearing -= 2 * M_PI;
		}

		float headingDegrees = bearing * (180.0 / M_PI);


		return headingDegrees;
	}
#endif
}

void RoverQMC5883L::setQMC5883LAddress (const int address)
{
	this->QMC5883L_ADDRESS = address;
}

void RoverQMC5883L::setQMC5883LDeclinationAngle (const float angle)
{
	this->DECLINATION_ANGLE = angle;
}

void RoverQMC5883L::setQMC5883LCalibrationPeriod(const int period)
{
	this->CALIBRATION_DURATION = period;
}

int RoverQMC5883L::getQMC5883LAddress (void)
{
	return this->QMC5883L_ADDRESS;
}

float RoverQMC5883L::getQMC5883LDeclinationAngle (void)
{
	return this->DECLINATION_ANGLE;
}

int RoverQMC5883L::getQMC5883LCalibrationPeriod (void)
{
	return this->CALIBRATION_DURATION;
}

template<typename T>
inline T RoverQMC5883L::MINIMUM_(const T& a, const T& b)
{
	return (a < b ? a : b);
}

template<typename T>
inline T RoverQMC5883L::MAXIMUM_(const T& a, const T& b)
{
	return (a > b ? a : b);
}
