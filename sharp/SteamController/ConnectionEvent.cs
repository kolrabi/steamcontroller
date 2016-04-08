using System;

namespace Kolrabi.SteamController
{
	public class ConnectionEvent: Event
	{
		public readonly WirelessState State;

		internal ConnectionEvent(ref SteamControllerLib.Event evt) : base(evt)
		{
			this.State = (WirelessState)evt.connection.details;
		}
	}
}

