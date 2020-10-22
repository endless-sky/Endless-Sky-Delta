/* Hardpoint.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HARDPOINT_H_
#define HARDPOINT_H_

#include "Angle.h"
#include "Point.h"

#include <utility>
#include <vector>

class Outfit;
class Projectile;
class Ship;
class Visual;



// A single weapon hardpoint on the ship (i.e. a gun port or turret mount),
// which may or may not have a weapon installed.
class Hardpoint {
public:
	// Constructor. Hardpoints may or may not specify what weapon is in them.
	// If isTurret is true, angles have various parameters according to the size:
	//  0: The turret is omnidirectional and points to the front when holding.
	//  1: The turret is omnidirectional and points to the angles[0] when holding.
	//  2: The turret can rotate in the range from angles[0] to angles[1] and points to the center of the range when holding.
	//  >=3: The turret can rotate in the range from angles[0] to angles[1] and points to the angles[2] when holding.
	// If isTurret is false, angles[0] specify the angle of firing direction.
	Hardpoint(const Point &point, const std::vector<Angle> &angles, bool isTurret, bool isParallel, const Outfit *outfit = nullptr);
	
	// Get the weapon installed in this hardpoint (or null if there is none).
	const Outfit *GetOutfit() const;
	// Get the location, relative to the center of the ship, from which
	// projectiles of this weapon should originate. This point must be
	// rotated to take the ship's current facing direction into account.
	const Point &GetPoint() const;
	// Get the angle that this weapon is aimed at, relative to the ship.
	const Angle &GetAngle() const;
	// Get the base angle that this weapon is aimed at (without harmonization/convergence), relative to the ship.
	const Angle &GetBaseAngle() const;
	// Get the angle of traverse. Return value is invalid if this is omnidirectional or a gun port.
	std::pair<Angle, Angle> GetAngleOfTraverse() const;
	// Get the angle this weapon ought to point at for ideal gun harmonization.
	Angle HarmonizedAngle() const;
	// Shortcuts for querying weapon characteristics.
	bool IsTurret() const;
	bool IsParallel() const;
	bool IsOmnidirectional() const;
	bool IsHoming() const;
	bool IsAntiMissile() const;
	bool CanAim() const;
	
	// Check if this weapon is ready to fire.
	bool IsReady() const;
	// Check if this weapon was firing in the previous step.
	bool WasFiring() const;
	// If this is a burst weapon, get the number of shots left in the burst.
	int BurstRemaining() const;
	// Perform one step (i.e. decrement the reload count).
	void Step();
	
	// Adjust this weapon's aim by the given amount, relative to its maximum
	// "turret turn" rate.
	void Aim(double amount);
	// Fire this weapon. If it is a turret, it automatically points toward
	// the given ship's target. If the weapon requires ammunition, it will
	// be subtracted from the given ship.
	void Fire(Ship &ship, std::vector<Projectile> &projectiles, std::vector<Visual> &visuals);
	// Fire an anti-missile. Returns true if the missile should be killed.
	bool FireAntiMissile(Ship &ship, const Projectile &projectile, std::vector<Visual> &visuals);
	
	// Install a weapon here (assuming it is empty). This is only for
	// Armament to call internally.
	void Install(const Outfit *outfit);
	// Reload this weapon.
	void Reload();
	// Uninstall the outfit from this port (if it has one).
	void Uninstall();
	
	// Get the angles that can be used as a parameter of the constructor when cloning this.
	const std::vector<Angle> &GetAnglesParameter() const;
	
	
private:
	// Reset the reload counters and expend ammunition, if any.
	void Fire(Ship &ship, const Point &start, const Angle &aim);
	
	// Update the angles of traverse.
	void UpdateAngleOfTraverse();
	
	
private:
	// The weapon installed in this hardpoint.
	const Outfit *outfit = nullptr;
	// Hardpoint location, in world coordinates relative to the ship's center.
	Point point;
	// Angle of firing direction (guns) or holding position (turret).
	Angle baseAngle;
	// The range where the weapon can traverse to point to the target (turret only).
	std::pair<Angle, Angle> angleOfTraverse;
	// The value of the angles parameter in the constructor.
	std::vector<Angle> anglesParameter;
	// This hardpoint is for a turret or a gun.
	bool isTurret = false;
	// Indicates if this hardpoint disallows converging (guns only).
	bool isParallel = false;
	// Indicates if this hardpoint is omnidirectional (turret only).
	bool isOmnidirectional = true;
	
	// Angle adjustment for convergence.
	Angle angle;
	// Reload timers and other attributes.
	double reload = 0.;
	double burstReload = 0.;
	int burstCount = 0;
	bool isFiring = false;
	bool wasFiring = false;
};



#endif
