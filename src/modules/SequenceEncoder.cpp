//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 4Bit Sequencer Encoder
//	Binary addressing for the 8 and 16 step sequencer channels
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#define SEQ_NUM_STEPS 0 // this lets the expander functionality know that either 8 or 16 step expanders are OK to use here

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerChannelMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequenceEncoder
#define PANEL_FILE "SequenceEncoder.svg"

struct SequenceEncoder : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	
	enum InputIds {
		A0_INPUT,
		A1_INPUT,
		A2_INPUT,
		A3_INPUT,
		CLOCK_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		NUM_OUTPUTS
	};
	
	enum LightIds {
		NUM_LIGHTS
	};
	

	GateProcessor gateClock;
	GateProcessor gateA0;
	GateProcessor gateA1;
	GateProcessor gateA2;
	GateProcessor gateA3;
	
	int count = 0;
	int length = SEQ_NUM_STEPS;
	float moduleVersion = 1.0f;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)

	SequenceEncoder() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(CLOCK_INPUT, "Clock");
		configInput(A0_INPUT, "Address bit 0");
		configInput(A1_INPUT, "Address bit 1");
		configInput(A2_INPUT, "Address bit 2");
		configInput(A3_INPUT, "Address bit 3");

		inputInfos[A0_INPUT]->description = "Least significant bit";
		inputInfos[A3_INPUT]->description = "Most significant bit";

		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		count = 0;
		length = SEQ_NUM_STEPS;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(moduleVersion));

		// add the theme details
		#include "../themes/dataToJson.hpp"
				
		return root;
	}

	void dataFromJson(json_t *root) override {
		
		json_t *version = json_object_get(root, "moduleVersion");

		if (version)
			moduleVersion = json_number_value(version);		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}

	void onReset() override {
		gateClock.reset();
		gateA0.reset();
		gateA1.reset();
		gateA2.reset();
		gateA3.reset();

		count = 0;
		length = SEQ_NUM_STEPS;
	}

	void process(const ProcessArgs &args) override {

		// process the clock
		gateClock.set(inputs[CLOCK_INPUT].getVoltage());

		// determine  count on positive clock edge
		if (gateClock.leadingEdge()) {
			count = 1;
			length = 0;

			if (inputs[A0_INPUT].isConnected()) {
				length = 2;
				
				if (gateA0.set(inputs[A0_INPUT].getVoltage()))
					count++;
			}
			
			if (inputs[A1_INPUT].isConnected()) {
				length = 4;
				
				if (gateA1.set(inputs[A1_INPUT].getVoltage()))
					count += 2;
			}
			
			if (inputs[A2_INPUT].isConnected()) {
				length = 8;
				
				if (gateA2.set(inputs[A2_INPUT].getVoltage()))
					count += 4;
			}
		
			if (inputs[A3_INPUT].isConnected()) {
				length = 16;
				
				if (gateA3.set(inputs[A3_INPUT].getVoltage()))
					count += 8;
			}
		}

		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				SequencerChannelMessage *messageToExpander = (SequencerChannelMessage*)(rightExpander.module->leftExpander.producerMessage);
				messageToExpander->set(count, length, gateClock.high(), true, 1, true); // always running

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
	}
};

struct SequenceEncoderWidget : ModuleWidget {

	std::string panelName;
	
	SequenceEncoderWidget(SequenceEncoder *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	

		// address inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, SequenceEncoder::A0_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, SequenceEncoder::A1_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, SequenceEncoder::A2_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, SequenceEncoder::A3_INPUT));
		
		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, SequenceEncoder::CLOCK_INPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		SequenceEncoder *module = dynamic_cast<SequenceEncoder*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};

Model *modelSequenceEncoder = createModel<SequenceEncoder, SequenceEncoderWidget>("SequenceEncoder");
