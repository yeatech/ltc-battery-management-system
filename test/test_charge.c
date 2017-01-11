#include "unity.h"
#include "unity_fixture.h"
#include <stdio.h>
#include "state_types.h"
#include "charge.h"

#define NUM_MODULES 2
#define TOTAL_CELLS NUM_MODULES*2
#define CELL_MAX 3600
#define CELL_MIN 2400
#define CHARGE_C_RATING 1
#define CELL_CAPACITY_CAh 100
#define CC_CHARGE_VOLTAGE CELL_MAX*TOTAL_CELLS
#define CC_CHARGE_CURRENT CELL_CAPACITY_CAh*CHARGE_C_RATING*10

BMS_INPUT_T input;
BMS_OUTPUT_T output;
BMS_STATE_T state;
BMS_CHARGER_STATUS_T _charger_status;
BMS_PACK_STATUS_T _pack_status;
PACK_CONFIG_T config;
uint8_t mod_cell_count[NUM_MODULES] = {2, 2};
uint32_t cell_voltages_mV[TOTAL_CELLS] = {3400, 3401, 3402, 3403};
BMS_CHARGE_REQ_T _charge_req;
bool balance_requests[NUM_MODULES];
BMS_ERROR_T bms_errors[NUM_MODULES];


void Test_Charge_SM_Shutdown(void);


TEST_GROUP(Charge_Test);

TEST_SETUP(Charge_Test) {
	printf("\r(Charge_Test)Setup");
	state.charger_status = &_charger_status;
	state.pack_config = &config;
  	
  	config.cell_min_mV = CELL_MIN;
	config.cell_max_mV = CELL_MAX;
	config.cell_capacity_cAh = CELL_CAPACITY_CAh;
	config.num_modules = NUM_MODULES;

	config.num_cells_in_modules = mod_cell_count;
	config.cell_charge_c_rating_cC = CHARGE_C_RATING*100;
	config.bal_on_thresh_mV = 4;
	config.bal_off_thresh_mV = 1;
	config.pack_cells_p = 1;

	input.mode_request = BMS_SSM_MODE_STANDBY;
	input.balance_mV = 0;
	input.contactors_closed = false;
	input.pack_status = &_pack_status;
	input.pack_status->cell_voltage_mV = cell_voltages_mV;

	output.charge_req = &_charge_req;
	output.balance_req = balance_requests;

	Charge_Init(&state);
	Charge_Config(&config);
	printf("...");
}

TEST_TEAR_DOWN(Charge_Test) {
	printf("...");
	input.mode_request = BMS_SSM_MODE_STANDBY;
	input.balance_mV = 0;
	input.contactors_closed = false;

	Charge_Step(&input, &state, &output);
	printf("Teardown\r\n");
}

TEST(Charge_Test, charge_off) {
	printf("off");
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);
}

TEST(Charge_Test, initialize) {
	printf("init");
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);

	input.mode_request = BMS_SSM_MODE_CHARGE;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_INIT);
	TEST_ASSERT_TRUE(output.close_contactors);


	input.mode_request = BMS_SSM_MODE_STANDBY;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);
	TEST_ASSERT_FALSE(output.close_contactors);
}

TEST(Charge_Test, to_cc) {
	printf("to_cc");
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);

	input.mode_request = BMS_SSM_MODE_CHARGE;
	input.contactors_closed = true;
	input.pack_status->pack_cell_min_mV = 3400;
	input.pack_status->pack_cell_max_mV = 3403;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_CC);
	TEST_ASSERT_TRUE(output.close_contactors);
	int i;
	for (i = 0; i < TOTAL_CELLS; i++)
		TEST_ASSERT_FALSE(output.balance_req[i]);
	TEST_ASSERT_TRUE(output.charge_req->charger_on);
	TEST_ASSERT_EQUAL(output.charge_req->charge_voltage_mV, CC_CHARGE_VOLTAGE);
	TEST_ASSERT_EQUAL(output.charge_req->charge_current_mA, CC_CHARGE_CURRENT);

	Test_Charge_SM_Shutdown();
}

TEST(Charge_Test, to_cc_with_balance) {
	printf("to_cc_w_bal");
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);

	input.mode_request = BMS_SSM_MODE_CHARGE;
	input.contactors_closed = true;
	input.pack_status->pack_cell_min_mV = 3400;
	input.pack_status->pack_cell_max_mV = 3405;
	cell_voltages_mV[TOTAL_CELLS - 1] = 3405;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_CC);
	TEST_ASSERT_TRUE(output.close_contactors);
	int i;
	for (i = 0; i < TOTAL_CELLS - 1; i++)
		TEST_ASSERT_FALSE(output.balance_req[i]);
	TEST_ASSERT_TRUE(output.balance_req[TOTAL_CELLS - 1]);
	TEST_ASSERT_TRUE(output.charge_req->charger_on);
	TEST_ASSERT_EQUAL(output.charge_req->charge_voltage_mV, CC_CHARGE_VOLTAGE);
	TEST_ASSERT_EQUAL(output.charge_req->charge_current_mA, CC_CHARGE_CURRENT);

	Test_Charge_SM_Shutdown();
}


TEST_GROUP_RUNNER(Charge_Test) {
	RUN_TEST_CASE(Charge_Test, charge_off);
	RUN_TEST_CASE(Charge_Test, initialize);
	RUN_TEST_CASE(Charge_Test, to_cc);
	RUN_TEST_CASE(Charge_Test, to_cc_with_balance);
}

void Test_Charge_SM_Shutdown(void) {
	int i;
	input.mode_request = BMS_SSM_MODE_STANDBY;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_DONE);
	TEST_ASSERT_FALSE(output.close_contactors);
	for (i = 0; i < TOTAL_CELLS; i++)
		TEST_ASSERT_FALSE(output.balance_req[i]);
	TEST_ASSERT_FALSE(output.charge_req->charger_on);
	// TEST_ASSERT_EQUAL(output.charge_req->charge_voltage_mV, 0);
	// TEST_ASSERT_EQUAL(output.charge_req->charge_current_mA, 0);

	input.contactors_closed = false;
	Charge_Step(&input, &state, &output);
	TEST_ASSERT_EQUAL(state.charge_state, BMS_CHARGE_OFF);
	TEST_ASSERT_FALSE(output.close_contactors);
	TEST_ASSERT_FALSE(output.charge_req->charger_on);
}

