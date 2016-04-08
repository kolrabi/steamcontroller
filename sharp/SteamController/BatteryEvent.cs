using System;

namespace Kolrabi.SteamController
{
	public class BatteryEvent : Event
	{
		public readonly ushort MilliVolts;

		internal BatteryEvent(ref SteamControllerLib.Event evt) : base(evt)
		{
			this.MilliVolts = evt.battery.voltage;
		}
	}
}

