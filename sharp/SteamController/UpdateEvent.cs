using System;

namespace Kolrabi.SteamController
{
	public class UpdateEvent : Event
	{
		public readonly ulong TimeStamp;
		public readonly ulong ButtonState;
		public readonly float LeftTrigger, RightTrigger;

		public readonly AxisPair LeftXY;
		public readonly AxisPair RightXY;

		public readonly Quat Orientation;
		public readonly Vector Acceleration;
		public readonly Vector AngularVelocity;

		internal UpdateEvent (ref SteamControllerLib.Event evt) : base(evt)
		{
			this.TimeStamp = evt.input.timeStamp;
			this.ButtonState = evt.input.buttons;
			this.LeftTrigger = evt.input.leftTrigger / 255.0f;
			this.RightTrigger= evt.input.rightTrigger/ 255.0f;

			this.LeftXY.x = evt.input.leftX / 32767.0f;
			this.LeftXY.y = evt.input.leftY / 32767.0f;
			this.RightXY.x = evt.input.rightX / 32767.0f;
			this.RightXY.y = evt.input.rightY / 32767.0f;

			this.Orientation.x = evt.input.qx / 32767.0f;
			this.Orientation.y = evt.input.qy / 32767.0f;
			this.Orientation.z = evt.input.qz / 32767.0f;
			this.Orientation.w = 1.0f - (float)Math.Sqrt(this.Orientation.x*this.Orientation.x+this.Orientation.y*this.Orientation.y+this.Orientation.z*this.Orientation.z);

			this.Acceleration.x = evt.input.ax / 32767.0f;
			this.Acceleration.y = evt.input.ay / 32767.0f;
			this.Acceleration.z = evt.input.az / 32767.0f;

			this.AngularVelocity.x = evt.input.gx / 32767.0f;
			this.AngularVelocity.y = evt.input.gy / 32767.0f;
			this.AngularVelocity.z = evt.input.gz / 32767.0f;
		}
	}
}

