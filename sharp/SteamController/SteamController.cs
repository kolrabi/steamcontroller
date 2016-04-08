using System;
using System.Collections.Generic;

namespace Kolrabi.SteamController
{
	public struct AxisPair { public float x, y; }
	public struct Vector   { public float x, y, z; }
	public struct Quat     { public float x, y, z, w; }

	public class SteamControllerDevice
	{
		public static SteamControllerDevice[] OpenControllers()
		{
			List<SteamControllerDevice> controllers = new List<SteamControllerDevice> ();

			IntPtr pEnum = SteamControllerLib.EnumControllerDevices ();
			while (pEnum != IntPtr.Zero) {
				IntPtr pDevice = SteamControllerLib.Open (pEnum);
				if (pDevice != IntPtr.Zero) {
					controllers.Add (new SteamControllerDevice (pDevice));
				}
				pEnum = SteamControllerLib.NextControllerDevice (pEnum);
			}

			return controllers.ToArray ();
		}

		public ulong LastUpdate { get; private set; }
		public Buttons ButtonState { get; private set; }
		public float LeftTrigger { get; private set; }
		public float RightTrigger { get; private set; }
		public AxisPair RightPad { get; private set; }
		public AxisPair LeftPad { get; private set; }
		public AxisPair Stick { get; private set; }
		public Quat Orientation { get; private set; }
		public Vector Acceleration { get; private set; }
		public Vector AngularVelocity { get; private set; }

		public ushort BatteryVoltage { get; private set; }
		public WirelessState State { get; private set; }

		public void Close() {
			SteamControllerLib.Close (this.pDevice);
			this.pDevice = IntPtr.Zero;
		}

		public bool IsWirelessDongle { 
			get { 
				return this.pDevice != IntPtr.Zero && SteamControllerLib.IsWirelessDongle (this.pDevice); 
			} 
		}

		public void TurnOff() {
			if (this.pDevice != IntPtr.Zero)
				SteamControllerLib.TurnOff (this.pDevice);
		}

		public WirelessState WirelessState { 
			get { 
				if (this.pDevice == IntPtr.Zero)
					return WirelessState.NotConnected;

				WirelessState state;
				SteamControllerLib.QueryWirelessState(this.pDevice, out state); 
				return WirelessState;
			} 
		}

		public void EnablePairing(bool enable) {
			if (this.pDevice != IntPtr.Zero)
				SteamControllerLib.EnablePairing (this.pDevice, enable, 0);
		}

		public void CommitPairing(bool connect) {
			if (this.pDevice != IntPtr.Zero)
				SteamControllerLib.CommitPairing (this.pDevice, connect);
		}

		public ConfigFlags Configuration {
			set {
				if (this.pDevice == IntPtr.Zero)
					return;
				SteamControllerLib.Configure (this.pDevice, (uint)value);
			}
		}

		public byte HomeButtonBrightness {
			set {
				if (this.pDevice == IntPtr.Zero)
					return;

				if (value > 100)
					value = 100;
				SteamControllerLib.SetHomeButtonBrightness(this.pDevice, value);
			}
		}

		public ushort Timeout {
			set {
				if (this.pDevice == IntPtr.Zero)
					return;
				SteamControllerLib.SetTimeout (this.pDevice, value);
			}
		}

		public Event ReadEvent()
		{
			if (this.pDevice == IntPtr.Zero)
				return null;

			SteamControllerLib.Event libEvent;
			if (!SteamControllerLib.ReadEvent (this.pDevice, out libEvent))
				return null;

			switch ((Event.EventTypeEnum)libEvent.eventType) {
			case Event.EventTypeEnum.Update:
				return new UpdateEvent (ref libEvent);
			case Event.EventTypeEnum.Connection:
				return new ConnectionEvent (ref libEvent);
			case Event.EventTypeEnum.Battery:
				return new BatteryEvent (ref libEvent);
			}
			return null;
		}

		public void UpdateState(Event evt)
		{
			if (evt == null)
				return;

			switch (evt.EventType) {
			case Event.EventTypeEnum.Update:
				UpdateEvent update = (UpdateEvent)evt;
				this.LastUpdate = update.TimeStamp;
				this.ButtonState = (Buttons)update.ButtonState;
				this.LeftTrigger = update.LeftTrigger;
				this.RightTrigger = update.RightTrigger;
				this.RightPad = update.RightXY;

				if ((this.ButtonState & Buttons.LFinger) != 0) {
					this.LeftPad = update.LeftXY;
				} else {
					this.Stick = update.LeftXY;
					if ((this.ButtonState & Buttons.PadStick) == 0) {
						this.LeftPad = new AxisPair ();
					}
				}				
				break;

			case Event.EventTypeEnum.Connection:
				ConnectionEvent connection = (ConnectionEvent)evt;
				this.State = (WirelessState)connection.State;
				break;

			case Event.EventTypeEnum.Battery:
				BatteryEvent battery = (BatteryEvent)evt;
				this.BatteryVoltage = battery.MilliVolts;
				break;
			}
		}

		public void TriggerHaptic(ushort motor, ushort onTime, ushort offTime, ushort count) {
			if (this.pDevice == IntPtr.Zero)
				return;

			SteamControllerLib.TriggerHaptic(this.pDevice, motor, onTime, offTime, count);
		}

		public void PlayMelody(uint id) {
			if (this.pDevice == IntPtr.Zero)
				return;

			SteamControllerLib.PlayMelody(this.pDevice, id);
		}

		private IntPtr pDevice;
		private SteamControllerDevice(IntPtr pDevice) {
			this.pDevice = pDevice;
		}

	}
}

