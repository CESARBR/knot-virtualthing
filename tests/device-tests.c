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
#include <knot/knot_protocol.h>
#include <ell/ell.h>

#include "src/device.h"
#include "src/device-pvt.h"

START_TEST(device_generate_new_id_is_not_empty)
{
	char fst[KNOT_PROTOCOL_UUID_LEN + 1];

	device_generate_new_id();
	strncpy(fst, device_get_id(), KNOT_PROTOCOL_UUID_LEN);
	fst[KNOT_PROTOCOL_UUID_LEN] = '\0';

	ck_assert_str_ne(fst, "\0");
}
END_TEST

START_TEST(device_generate_new_id_is_different_from_previous)
{
	char fst[KNOT_PROTOCOL_UUID_LEN + 1];
	char snd[KNOT_PROTOCOL_UUID_LEN + 1];

	device_generate_new_id();
	strncpy(fst, device_get_id(), KNOT_PROTOCOL_UUID_LEN);
	fst[KNOT_PROTOCOL_UUID_LEN] = '\0';

	device_generate_new_id();
	strncpy(snd, device_get_id(), KNOT_PROTOCOL_UUID_LEN);
	snd[KNOT_PROTOCOL_UUID_LEN] = '\0';

	ck_assert_str_ne(fst, snd);
}
END_TEST

Suite *device_suite(void)
{
	Suite *dvc_suite;
	TCase *tc_generate_id;

	dvc_suite = suite_create("Device");

	/* Generate ID test case */
	tc_generate_id = tcase_create("Generate ID");
	tcase_add_test(tc_generate_id,
		       device_generate_new_id_is_different_from_previous);
	tcase_add_test(tc_generate_id, device_generate_new_id_is_not_empty);

	suite_add_tcase(dvc_suite, tc_generate_id);

	return dvc_suite;
}

int main(void)
{
	int number_failed;
	Suite *dvc_suite;
	SRunner *dvc_suite_runner;

	dvc_suite = device_suite();
	dvc_suite_runner = srunner_create(dvc_suite);

	srunner_run_all(dvc_suite_runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(dvc_suite_runner);
	srunner_free(dvc_suite_runner);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
