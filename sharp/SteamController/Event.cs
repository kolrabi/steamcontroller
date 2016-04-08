using System;

namespace Kolrabi.SteamController
{
	public class Event
	{
		public enum EventTypeEnum {
			Update = 1,
			Connection = 3,
			Battery = 4
		}

		public readonly EventTypeEnum EventType;

		internal Event (SteamControllerLib.Event evt)
		{
			this.EventType = (EventTypeEnum)evt.eventType;
		}
	}
}

