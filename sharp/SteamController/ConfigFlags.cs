using System;

namespace Kolrabi.SteamController
{
	[Flags]
	public enum ConfigFlags {

		HorizontalAccelerationAsStick = 1,
		VerticalAccelerationAsStick   = 2,
		SendOrientation               = 4,
		SendAcceleration              = 8,
		SendGyro                      = 16,
		SendBatteryStatus             = 32,
		RightPadHapticTouch           = 64,
		RightPadHapticTrackball       = 128,
		StickHaptic                   = 256
	}
}

