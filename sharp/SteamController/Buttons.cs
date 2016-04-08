using System;

namespace Kolrabi.SteamController
{
	[Flags]
	public enum Buttons {
		RT          = (1<<0),        /**< Right trigger fully pressed. */
		LT          = (1<<1),        /**< Left trigger fully pressed. */
		RS          = (1<<2),        /**< Right shoulder button pressed. */
		LS          = (1<<3),        /**< Left shoulder button pressed. */
		Y           = (1<<4),        /**< Y button. */
		B           = (1<<5),        /**< B button. */
		X           = (1<<6),        /**< X button. */
		A           = (1<<7),        /**< A button. */

		DPad_Up     = (0x01 << 8),   /**< Left pad pressed with thumb in the upper quarter. */
		DPad_Right  = (0x02 << 8),   /**< Left pad pressed with thumb in the right quarter. */
		DPad_Left   = (0x04 << 8),   /**< Left pad pressed with thumb in the left quarter. */
		DPad_Down   = (0x08 << 8),   /**< Left pad pressed with thumb in the bottom quarter. */
		Prev        = (0x10 << 8),   /**< Left arrow button. */
		Home        = (0x20 << 8),   /**< Steam logo button. */
		Next        = (0x40 << 8),   /**< Right arrow button. */
		LG          = (0x80 << 8),   /**< Left grip button. */

		RG          = (0x01 << 16),  /**< Right grip button. */
		Stick       = (0x02 << 16),  /**< Stick is pressed down. */
		RPad        = (0x04 << 16),  /**< Right pad pressed. */
		LFinger     = (0x08 << 16),  /**< If set, a finger is touching left touch pad. */
		RFinger     = (0x10 << 16),  /**< If set, a finger is touching right touch pad. */

		PadStick    = (0x80 << 16),  /**< If set, LFINGER to determines whether leftXY is 
		pad position or stick position. */
	}

}

