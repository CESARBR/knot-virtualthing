/**
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2020, CESAR. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <check.h>
#include <stdlib.h>

#include "src/sm-pvt.h"
#include "mocks/fake-device.h"

START_TEST(disconnected_get_next_event_ready_is_register)
{
	device_set_has_cred_rc(0);
	int next_state = get_next_disconnected(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(disconnected_get_next_event_ready_is_auth)
{
	device_set_has_cred_rc(1);
	int next_state = get_next_disconnected(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(disconnected_get_next_event_not_ready_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_timeout_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_reg_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_reg_not_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_auth_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_auth_not_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_schema_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_schema_not_ok_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_unreg_req_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_data_update_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_publish_data_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(disconnected_get_next_event_reg_perm_is_disconnected)
{
	int next_state = get_next_disconnected(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(register_get_next_event_ready_is_register)
{
	int next_state = get_next_register(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_not_ready_is_disconnected)
{
	int next_state = get_next_register(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(register_get_next_event_timeout_is_register)
{
	int next_state = get_next_register(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_reg_ok_is_auth)
{
	int next_state = get_next_register(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(register_get_next_event_reg_not_ok_is_register)
{
	int next_state = get_next_register(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_auth_ok_is_register)
{
	int next_state = get_next_register(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_auth_not_ok_is_register)
{
	int next_state = get_next_register(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_schema_ok_is_register)
{
	int next_state = get_next_register(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_schema_not_ok_is_register)
{
	int next_state = get_next_register(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_unreg_req_is_register)
{
	int next_state = get_next_register(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_data_update_is_register)
{
	int next_state = get_next_register(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_publish_data_is_register)
{
	int next_state = get_next_register(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(register_get_next_event_reg_perm_is_register)
{
	int next_state = get_next_register(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

START_TEST(auth_get_next_event_ready_is_auth)
{
	int next_state = get_next_auth(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_not_ready_is_disconnected)
{
	int next_state = get_next_auth(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(auth_get_next_event_timeout_is_auth)
{
	int next_state = get_next_auth(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_reg_ok_is_auth)
{
	int next_state = get_next_auth(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_reg_not_ok_is_auth)
{
	int next_state = get_next_auth(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_auth_ok_is_online)
{
	device_set_schema_change_rc(0);
	int next_state = get_next_auth(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(auth_get_next_event_auth_ok_is_schema)
{
	device_set_schema_change_rc(1);
	int next_state = get_next_auth(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(auth_get_next_event_auth_not_ok_is_unregister)
{
	int next_state = get_next_auth(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(auth_get_next_event_schema_ok_is_auth)
{
	int next_state = get_next_auth(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_schema_not_ok_is_auth)
{
	int next_state = get_next_auth(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_unreg_req_is_unregister)
{
	int next_state = get_next_auth(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(auth_get_next_event_data_update_is_auth)
{
	int next_state = get_next_auth(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_publish_data_is_auth)
{
	int next_state = get_next_auth(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(auth_get_next_event_reg_perm_is_auth)
{
	int next_state = get_next_auth(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_AUTH);
}
END_TEST

START_TEST(schema_get_next_event_ready_is_schema)
{
	int next_state = get_next_schema(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_not_ready_is_disconnected)
{
	int next_state = get_next_schema(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(schema_get_next_event_timeout_is_schema)
{
	int next_state = get_next_schema(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_reg_ok_is_schema)
{
	int next_state = get_next_schema(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_reg_not_ok_is_schema)
{
	int next_state = get_next_schema(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_auth_ok_is_schema)
{
	int next_state = get_next_schema(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_auth_not_ok_is_schema)
{
	int next_state = get_next_schema(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_schema_ok_is_online)
{
	int next_state = get_next_schema(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(schema_get_next_event_schema_not_ok_is_error)
{
	int next_state = get_next_schema(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_ERROR);
}
END_TEST

START_TEST(schema_get_next_event_unreg_req_is_unregister)
{
	int next_state = get_next_schema(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(schema_get_next_event_data_update_is_schema)
{
	int next_state = get_next_schema(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_publish_data_is_schema)
{
	int next_state = get_next_schema(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(schema_get_next_event_reg_perm_is_schema)
{
	int next_state = get_next_schema(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_SCHEMA);
}
END_TEST

START_TEST(online_get_next_event_ready_is_online)
{
	int next_state = get_next_online(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_not_ready_is_disconnected)
{
	int next_state = get_next_online(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_DISCONNECTED);
}
END_TEST

START_TEST(online_get_next_event_timeout_is_online)
{
	int next_state = get_next_online(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_reg_ok_is_online)
{
	int next_state = get_next_online(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_reg_not_ok_is_online)
{
	int next_state = get_next_online(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_auth_ok_is_online)
{
	int next_state = get_next_online(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_auth_not_ok_is_online)
{
	int next_state = get_next_online(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_schema_ok_is_online)
{
	int next_state = get_next_online(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_schema_not_ok_is_online)
{
	int next_state = get_next_online(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_unreg_req_is_unregister)
{
	int next_state = get_next_online(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(online_get_next_event_data_update_is_online)
{
	int next_state = get_next_online(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_publish_data_is_online)
{
	int next_state = get_next_online(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(online_get_next_event_reg_perm_is_online)
{
	int next_state = get_next_online(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_ONLINE);
}
END_TEST

START_TEST(unregister_get_next_event_ready_is_unregister)
{
	int next_state = get_next_unregister(EVT_READY, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_not_ready_is_unregister)
{
	int next_state = get_next_unregister(EVT_NOT_READY, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_timeout_is_unregister)
{
	int next_state = get_next_unregister(EVT_TIMEOUT, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_reg_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_REG_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_reg_not_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_REG_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_auth_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_AUTH_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_auth_not_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_AUTH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_schema_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_SCH_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_schema_not_ok_is_unregister)
{
	int next_state = get_next_unregister(EVT_SCH_NOT_OK, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_unreg_req_is_unregister)
{
	int next_state = get_next_unregister(EVT_UNREG_REQ, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_data_update_is_unregister)
{
	int next_state = get_next_unregister(EVT_DATA_UPDT, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_publish_data_is_unregister)
{
	int next_state = get_next_unregister(EVT_PUB_DATA, NULL);
	ck_assert_int_eq(next_state, ST_UNREGISTER);
}
END_TEST

START_TEST(unregister_get_next_event_reg_perm_is_register)
{
	int next_state = get_next_unregister(EVT_REG_PERM, NULL);
	ck_assert_int_eq(next_state, ST_REGISTER);
}
END_TEST

static void add_disconnected_state_test_case(Suite *sm_suite)
{
	TCase *tc_disconnected;

	/* Disconnected test cases */
	tc_disconnected = tcase_create("Disconnected");
	tcase_add_test(tc_disconnected, disconnected_get_next_event_ready_is_register);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_ready_is_auth);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_not_ready_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_timeout_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_reg_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_reg_not_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_auth_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_auth_not_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_schema_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_schema_not_ok_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_unreg_req_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_data_update_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_publish_data_is_disconnected);
	tcase_add_test(tc_disconnected, disconnected_get_next_event_reg_perm_is_disconnected);

	suite_add_tcase(sm_suite, tc_disconnected);
}

static void add_register_state_test_case(Suite *sm_suite)
{
	TCase *tc_register;

	/* Register test case */
	tc_register = tcase_create("Register");
	tcase_add_test(tc_register, register_get_next_event_ready_is_register);
	tcase_add_test(tc_register, register_get_next_event_not_ready_is_disconnected);
	tcase_add_test(tc_register, register_get_next_event_timeout_is_register);
	tcase_add_test(tc_register, register_get_next_event_reg_ok_is_auth);
	tcase_add_test(tc_register, register_get_next_event_reg_not_ok_is_register);
	tcase_add_test(tc_register, register_get_next_event_auth_ok_is_register);
	tcase_add_test(tc_register, register_get_next_event_auth_not_ok_is_register);
	tcase_add_test(tc_register, register_get_next_event_schema_ok_is_register);
	tcase_add_test(tc_register, register_get_next_event_schema_not_ok_is_register);
	tcase_add_test(tc_register, register_get_next_event_unreg_req_is_register);
	tcase_add_test(tc_register, register_get_next_event_data_update_is_register);
	tcase_add_test(tc_register, register_get_next_event_publish_data_is_register);
	tcase_add_test(tc_register, register_get_next_event_reg_perm_is_register);

	suite_add_tcase(sm_suite, tc_register);
}

static void add_auth_state_test_case(Suite *sm_suite)
{
	TCase *tc_auth;

	/* Auth test case */
	tc_auth = tcase_create("Auth");
	tcase_add_test(tc_auth, auth_get_next_event_ready_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_not_ready_is_disconnected);
	tcase_add_test(tc_auth, auth_get_next_event_timeout_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_reg_ok_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_reg_not_ok_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_auth_ok_is_online);
	tcase_add_test(tc_auth, auth_get_next_event_auth_ok_is_schema);
	tcase_add_test(tc_auth, auth_get_next_event_auth_not_ok_is_unregister);
	tcase_add_test(tc_auth, auth_get_next_event_schema_ok_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_schema_not_ok_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_unreg_req_is_unregister);
	tcase_add_test(tc_auth, auth_get_next_event_data_update_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_publish_data_is_auth);
	tcase_add_test(tc_auth, auth_get_next_event_reg_perm_is_auth);

	suite_add_tcase(sm_suite, tc_auth);
}

static void add_schema_state_test_case(Suite *sm_suite)
{
	TCase *tc_schema;
	/* Schema test case */
	tc_schema = tcase_create("Schema");
	tcase_add_test(tc_schema, schema_get_next_event_ready_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_not_ready_is_disconnected);
	tcase_add_test(tc_schema, schema_get_next_event_timeout_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_reg_ok_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_reg_not_ok_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_auth_ok_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_auth_not_ok_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_schema_ok_is_online);
	tcase_add_test(tc_schema, schema_get_next_event_schema_not_ok_is_error);
	tcase_add_test(tc_schema, schema_get_next_event_unreg_req_is_unregister);
	tcase_add_test(tc_schema, schema_get_next_event_data_update_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_publish_data_is_schema);
	tcase_add_test(tc_schema, schema_get_next_event_reg_perm_is_schema);

	suite_add_tcase(sm_suite, tc_schema);
}


static void add_online_state_test_case(Suite *sm_suite)
{
	TCase *tc_online;

	/* Online test case */
	tc_online = tcase_create("Online");
	tcase_add_test(tc_online, online_get_next_event_ready_is_online);
	tcase_add_test(tc_online, online_get_next_event_not_ready_is_disconnected);
	tcase_add_test(tc_online, online_get_next_event_timeout_is_online);
	tcase_add_test(tc_online, online_get_next_event_reg_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_reg_not_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_auth_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_auth_not_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_schema_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_schema_not_ok_is_online);
	tcase_add_test(tc_online, online_get_next_event_unreg_req_is_unregister);
	tcase_add_test(tc_online, online_get_next_event_data_update_is_online);
	tcase_add_test(tc_online, online_get_next_event_publish_data_is_online);
	tcase_add_test(tc_online, online_get_next_event_reg_perm_is_online);

	suite_add_tcase(sm_suite, tc_online);
}

static void add_unregister_state_test_case(Suite *sm_suite)
{
	TCase *tc_unregister;

	/* Unregister test case */
	tc_unregister = tcase_create("Unregister");
	tcase_add_test(tc_unregister, unregister_get_next_event_ready_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_not_ready_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_timeout_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_reg_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_reg_not_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_auth_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_auth_not_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_schema_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_schema_not_ok_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_unreg_req_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_data_update_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_publish_data_is_unregister);
	tcase_add_test(tc_unregister, unregister_get_next_event_reg_perm_is_register);

	suite_add_tcase(sm_suite, tc_unregister);
}

Suite *state_machine_suite(void)
{
	Suite *sm_suite;

	sm_suite = suite_create("State Machine");

	add_disconnected_state_test_case(sm_suite);
	add_register_state_test_case(sm_suite);
	add_auth_state_test_case(sm_suite);
	add_schema_state_test_case(sm_suite);
	add_online_state_test_case(sm_suite);
	add_unregister_state_test_case(sm_suite);

	return sm_suite;
}

int main(void)
{
	int number_failed;
	Suite *sm_suite;
	SRunner *sm_suite_runner;

	sm_suite = state_machine_suite();
	sm_suite_runner = srunner_create(sm_suite);

	srunner_run_all(sm_suite_runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sm_suite_runner);
	srunner_free(sm_suite_runner);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
