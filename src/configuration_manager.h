#ifndef __CONFIGURATION_MANAGER_H__
#define __CONFIGURATION_MANAGER_H__

#include <gconfmm.h>
#include "me-tv.h"

class ConfigurationManager
{
private:
	Glib::RefPtr<Gnome::Conf::Client>	gconf_client;

	Glib::ustring get_path(const Glib::ustring& key);

public:
	void initialise();
	
	void set_string_default(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_default(const Glib::ustring& key, gint value);
	void set_boolean_default(const Glib::ustring& key, gboolean value);	
	
	StringList		get_string_list_value(const Glib::ustring& key);
	Glib::ustring	get_string_value(const Glib::ustring& key);
	gint			get_int_value(const Glib::ustring& key);
	gboolean		get_boolean_value(const Glib::ustring& key);

	void set_string_list_value(const Glib::ustring& key, const StringList& value);
	void set_string_value(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_value(const Glib::ustring& key, gint value);
	void set_boolean_value(const Glib::ustring& key, gboolean value);
};

#endif