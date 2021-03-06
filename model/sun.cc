/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' Mediterranea di Reggio Calabria (UNIRC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Giovanni Benigno <giovanni.benigno.954@studenti.unirc.it>
 *         Orazio Briante <orazio.briante@unirc.it>
 */

#include "sun.h"


#include <ctime>
#include <math.h>

namespace ns3 {

double
Sun::GetIncidentInsolation (const tm *date, const double &latitude, const double &longitude, const double &altitude)
{
  Sun::Coordinates coordinates;
  Sun::PSA (date, latitude, longitude, &coordinates);
  if (coordinates.dElevationAngle > 0)
    {
      return GetAirMass (latitude, altitude) * (sin (coordinates.dElevationAngle * rad) / rad) * 1e3;
      //return GetAirMass (latitude);
    }

  return 0;
}

double
Sun::DecimalHours (const tm *date)
{
  return (double) ((date->tm_hour + 1 + (date->tm_min + date->tm_sec / SECONDS_IN_MINUTE ) / MINUTES_IN_HOUR) - date->tm_gmtoff / SECONDS_IN_HOUR);
}

void
Sun::PSA (const tm *date, const double &latitude, const double &longitude, Sun::Coordinates* udtSunCoordinates)
{

  double dDecimalHours = Sun::DecimalHours (date);

  int year = date->tm_year + 1900;
  int month = date->tm_mon + 1;
  int day = date->tm_mday;

  // Main variables
  double dElapsedJulianDays;
  double dEclipticLongitude;
  double dEclipticObliquity;
  double dRightAscension;
  double dDeclination;

  // Auxiliary variables
  double dY;
  double dX;

  // Calculate difference in days between the current Julian Day
  // and JD 2451545.0, which is noon 1 January 2000 Universal Time
  {
    double dJulianDate;
    long int liAux1;
    long int liAux2;

    // Calculate current Julian Day
    liAux1 = (month - 14) / 12;
    liAux2 = (1461 * (year + 4800 + liAux1)) / 4 + (367 * (month
                                                           - 2 - 12 * liAux1)) / 12 - (3 * ((year + 4900
                                                                                             + liAux1) / 100)) / 4 + day - 32075;
    dJulianDate = (double)(liAux2) - 0.5 + dDecimalHours / 24.0;
    // Calculate difference between current Julian Day and JD 2451545.0
    dElapsedJulianDays = dJulianDate - 2451545.0;
  }

  // Calculate ecliptic coordinates (ecliptic longitude and obliquity of the
  // ecliptic in radians but without limiting the angle to be less than 2*Pi
  // (i.e., the result may be greater than 2*Pi)
  {
    double dMeanLongitude;
    double dMeanAnomaly;
    double dOmega;
    dOmega = 2.1429 - 0.0010394594 * dElapsedJulianDays;
    dMeanLongitude = 4.8950630 + 0.017202791698 * dElapsedJulianDays;             // Radians
    dMeanAnomaly = 6.2400600 + 0.0172019699 * dElapsedJulianDays;
    dEclipticLongitude = dMeanLongitude + 0.03341607 * sin ( dMeanAnomaly )
      + 0.00034894 * sin ( 2 * dMeanAnomaly ) - 0.0001134
      - 0.0000203 * sin (dOmega);
    dEclipticObliquity = 0.4090928 - 6.2140e-9 * dElapsedJulianDays
      + 0.0000396 * cos (dOmega);
  }

  // Calculate celestial coordinates ( right ascension and declination ) in radians
  // but without limiting the angle to be less than 2*Pi (i.e., the result may be
  // greater than 2*Pi)
  {
    double dSin_EclipticLongitude;
    dSin_EclipticLongitude = sin ( dEclipticLongitude );
    dY = cos ( dEclipticObliquity ) * dSin_EclipticLongitude;
    dX = cos ( dEclipticLongitude );
    dRightAscension = atan2 ( dY,dX );
    if ( dRightAscension < 0.0 )
      {
        dRightAscension = dRightAscension + twopi;
      }
    dDeclination = asin ( sin ( dEclipticObliquity ) * dSin_EclipticLongitude );
  }

  // Calculate local coordinates ( azimuth and zenith angle ) in degrees
  {
    double dGreenwichMeanSiderealTime;
    double dLocalMeanSiderealTime;
    double dLatitudeInRadians;
    double dHourAngle;
    double dCos_Latitude;
    double dSin_Latitude;
    double dCos_HourAngle;
    double dParallax;
    dGreenwichMeanSiderealTime = 6.6974243242 +
      0.0657098283 * dElapsedJulianDays
      + dDecimalHours;
    dLocalMeanSiderealTime = (dGreenwichMeanSiderealTime * 15
                              + longitude) * rad;
    dHourAngle = dLocalMeanSiderealTime - dRightAscension;
    dLatitudeInRadians = latitude * rad;
    dCos_Latitude = cos ( dLatitudeInRadians );
    dSin_Latitude = sin ( dLatitudeInRadians );
    dCos_HourAngle = cos ( dHourAngle );
    udtSunCoordinates->dZenithAngle = (acos ( dCos_Latitude * dCos_HourAngle
                                              * cos (dDeclination) + sin ( dDeclination ) * dSin_Latitude));
    dY = -sin ( dHourAngle );
    dX = tan ( dDeclination ) * dCos_Latitude - dSin_Latitude * dCos_HourAngle;
    udtSunCoordinates->dAzimuth = atan2 ( dY, dX );
    if ( udtSunCoordinates->dAzimuth < 0.0 )
      {
        udtSunCoordinates->dAzimuth = udtSunCoordinates->dAzimuth + twopi;
      }
    udtSunCoordinates->dAzimuth = udtSunCoordinates->dAzimuth / rad;
    // Parallax Correction
    dParallax = (dEarthMeanRadius / dAstronomicalUnit)
      * sin (udtSunCoordinates->dZenithAngle);
    udtSunCoordinates->dZenithAngle = (udtSunCoordinates->dZenithAngle
                                       + dParallax) / rad;

    udtSunCoordinates->dElevationAngle = 90 - udtSunCoordinates->dZenithAngle;
  }
}

double
Sun::GetAirMass (const double &latitude, const double &altitude)
{
  double AM = 1 / cos (latitude * rad);
  double a = 0.14;

  return 1.353 * ((1 - a * altitude) * pow (0.7, pow (AM, 0.678)) + a * altitude);
}

} /* namespace ns3 */


