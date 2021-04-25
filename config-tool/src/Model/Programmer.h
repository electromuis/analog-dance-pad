#ifndef slic3r_FirmwareDialog_hpp_
#define slic3r_FirmwareDialog_hpp_

#include <memory>

#include <wx/dialog.h>
#include "GUI_Utils.hpp"


namespace mpc {


	class FirmwareDialog : public GUI::DPIDialog
	{
		enum {
			DIALOG_MARGIN = 15,
			SPACING = 10,
			MIN_WIDTH = 50,
			MIN_HEIGHT = /*18*/25,
			MIN_HEIGHT_EXPANDED = 40,
		};

	public:
		FirmwareDialog(wxWindow* parent);
		FirmwareDialog(FirmwareDialog&&) = delete;
		FirmwareDialog(const FirmwareDialog&) = delete;
		FirmwareDialog& operator=(FirmwareDialog&&) = delete;
		FirmwareDialog& operator=(const FirmwareDialog&) = delete;
		~FirmwareDialog();

		static void run(wxWindow* parent);

	private:
		struct priv;
		std::unique_ptr<priv> p;
	};


}

#endif
