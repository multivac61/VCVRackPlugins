//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack	
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

#include "CountModula.hpp"

Plugin *pluginInstance;
int countmodula_defaultTheme = 0;
int prevDefaultTheme = -1;


void countmodula_init(Plugin *p) {
	pluginInstance = p;

	countmodula_defaultTheme = readDefaultIntegerValue("DefaultTheme");
	prevDefaultTheme = -1;
	
#include "InitialiseModels.hpp"
}

void setDefaultTheme(int themeToUse, bool previous) {
	if (previous)
		prevDefaultTheme = themeToUse;
	else
		countmodula_defaultTheme = themeToUse;
}

int getDefaultTheme(bool previous) {
	return previous ? prevDefaultTheme : countmodula_defaultTheme;
}


// save the given global count modula settings`
void saveSettings(json_t *rootJ) {
	std::string settingsFilename = asset::user("CountModula.json");
	
	FILE *file = fopen(settingsFilename.c_str(), "w");
	
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
}

// read the global count modula settings
json_t * readSettings() {
	std::string settingsFilename = asset::user("CountModula.json");
	FILE *file = fopen(settingsFilename.c_str(), "r");
	
	if (!file) {
		return json_object();
	}
	
	json_error_t error;
	json_t *rootJ = json_loadf(file, 0, &error);
	
	fclose(file);
	return rootJ;
}

// read the given default integer value from the global count modula settings file
int readDefaultIntegerValue(std::string setting) {
	int value = 0; // default to the standard value
	
	// read the settings file
	json_t *rootJ = readSettings();
	
	// get the default value
	json_t* jsonValue = json_object_get(rootJ, setting.c_str());
	if (jsonValue)
		value = json_integer_value(jsonValue);

	// houskeeping
	json_decref(rootJ);
	
	return value;
}

// save the given integer value in the global count modula settings file
void saveDefaultIntegerValue(std::string setting, int value) {
	// read the settings file
	json_t *rootJ = readSettings();
	
	// set the default theme value
	json_object_set_new(rootJ, setting.c_str(), json_integer(value));

	// save the updated data
	saveSettings(rootJ);
	
	// houskeeping
	json_decref(rootJ);
}



// hack for module expanders always to the right
static math::Vec eachNearestGridPosRight(math::Vec pos, std::function<bool(math::Vec pos)> f) {
	math::Vec leftPos = (pos / RACK_GRID_SIZE).round();
	math::Vec rightPos = leftPos + math::Vec(1, 0);

	while (true) {
		if (f(rightPos * RACK_GRID_SIZE))
			return rightPos * RACK_GRID_SIZE;
		rightPos.x += 1;
	}

	assert(false);
	return math::Vec();
}

// hack for module expanders always to the left
void setModulePosNearestRight(ModuleWidget* mw, math::Vec pos) {
	eachNearestGridPosRight(pos, [&](math::Vec pos) -> bool {
		return APP->scene->rack->requestModulePos(mw, pos);
	});
}

static math::Vec eachNearestGridPosLeft(math::Vec pos, std::function<bool(math::Vec pos)> f) {
	math::Vec leftPos = (pos / RACK_GRID_SIZE).round();

	while (true) {
		if (f(leftPos * RACK_GRID_SIZE))
			return leftPos * RACK_GRID_SIZE;
		leftPos.x -= 1;
	}

	assert(false);
	return math::Vec();
}

// hack for module expanders always to the left
void setModulePosNearestLeft(ModuleWidget* mw, math::Vec pos) {
	eachNearestGridPosLeft(pos, [&](math::Vec pos) -> bool {
		return APP->scene->rack->requestModulePos(mw, pos);
	});
}



