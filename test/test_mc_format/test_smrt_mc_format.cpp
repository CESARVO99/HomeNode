/**
 * @file    test_smrt_mc_format.cpp
 * @brief   Unit tests for smrt_mc_format module (data conversion utilities)
 * @project HOMENODE
 * @version 1.0.0
 *
 * Tests run natively on the PC using PlatformIO's Unity framework.
 * Execute with: pio test -e native
 */

//-----------------------------------------------------------------------------
// Includes
// Note: UNIT_TEST is defined via build_flags in platformio.ini,
//       which disables the exclude_* guards in smrt_mc_format.h,
//       allowing all functions to be compiled and tested.
//-----------------------------------------------------------------------------
#include <unity.h>
#include <string.h>

#include "smrt_mc_format.h"

// Include the source file directly to compile all functions in test context.
// This avoids PlatformIO build_src_filter issues with native test builds.
#include "../../src/core/smrt_mc_format.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {}
void tearDown(void) {}

//=============================================================================
// Test: SMRT_TYPE_SWAP_16 macro
//=============================================================================

void test_macro_swap16_basic(void) {
    uint16 val = 0x1234;
    uint16 result = SMRT_TYPE_SWAP_16(val);
    TEST_ASSERT_EQUAL_HEX16(0x3412, result);
}

void test_macro_swap16_zero(void) {
    uint16 result = SMRT_TYPE_SWAP_16(0x0000);
    TEST_ASSERT_EQUAL_HEX16(0x0000, result);
}

void test_macro_swap16_ff(void) {
    uint16 result = SMRT_TYPE_SWAP_16(0xFFFF);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, result);
}

//=============================================================================
// Test: SMRT_TYPE_SWAP_32 macro
//=============================================================================

void test_macro_swap32_basic(void) {
    uint32 val = 0x12345678;
    uint32 result = SMRT_TYPE_SWAP_32(val);
    TEST_ASSERT_EQUAL_HEX32(0x78563412, result);
}

void test_macro_swap32_zero(void) {
    uint32 result = SMRT_TYPE_SWAP_32(0x00000000);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, result);
}

//=============================================================================
// Test: SMRT_TO_UPPER / SMRT_TO_LOWER macros
//=============================================================================

void test_macro_to_upper_lowercase(void) {
    TEST_ASSERT_EQUAL_CHAR('A', SMRT_TO_UPPER('a'));
    TEST_ASSERT_EQUAL_CHAR('Z', SMRT_TO_UPPER('z'));
    TEST_ASSERT_EQUAL_CHAR('M', SMRT_TO_UPPER('m'));
}

void test_macro_to_upper_already_upper(void) {
    TEST_ASSERT_EQUAL_CHAR('A', SMRT_TO_UPPER('A'));
    TEST_ASSERT_EQUAL_CHAR('Z', SMRT_TO_UPPER('Z'));
}

void test_macro_to_upper_non_alpha(void) {
    TEST_ASSERT_EQUAL_CHAR('0', SMRT_TO_UPPER('0'));
    TEST_ASSERT_EQUAL_CHAR(' ', SMRT_TO_UPPER(' '));
    TEST_ASSERT_EQUAL_CHAR('.', SMRT_TO_UPPER('.'));
}

void test_macro_to_lower_uppercase(void) {
    TEST_ASSERT_EQUAL_CHAR('a', SMRT_TO_LOWER('A'));
    TEST_ASSERT_EQUAL_CHAR('z', SMRT_TO_LOWER('Z'));
    TEST_ASSERT_EQUAL_CHAR('m', SMRT_TO_LOWER('M'));
}

void test_macro_to_lower_already_lower(void) {
    TEST_ASSERT_EQUAL_CHAR('a', SMRT_TO_LOWER('a'));
    TEST_ASSERT_EQUAL_CHAR('z', SMRT_TO_LOWER('z'));
}

//=============================================================================
// Test: SMRT_MIN / SMRT_MAX macros
//=============================================================================

void test_macro_min(void) {
    TEST_ASSERT_EQUAL(3, SMRT_MIN(3, 5));
    TEST_ASSERT_EQUAL(3, SMRT_MIN(5, 3));
    TEST_ASSERT_EQUAL(7, SMRT_MIN(7, 7));
}

void test_macro_max(void) {
    TEST_ASSERT_EQUAL(5, SMRT_MAX(3, 5));
    TEST_ASSERT_EQUAL(5, SMRT_MAX(5, 3));
    TEST_ASSERT_EQUAL(7, SMRT_MAX(7, 7));
}

//=============================================================================
// Test: smrtTypeSetHexadecimal
//=============================================================================

void test_set_hex_single_digit(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetHexadecimal(buf, 0x0A, 1);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_CHAR('A', buf[0]);
}

void test_set_hex_two_digits(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetHexadecimal(buf, 0xFF, 2);
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL_CHAR('F', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('F', buf[1]);
}

void test_set_hex_four_digits(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetHexadecimal(buf, 0x1A2B, 4);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_STRING_LEN("1A2B", buf, 4);
}

void test_set_hex_eight_digits(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetHexadecimal(buf, 0xDEADBEEF, 8);
    TEST_ASSERT_EQUAL(8, len);
    TEST_ASSERT_EQUAL_STRING_LEN("DEADBEEF", buf, 8);
}

void test_set_hex_zero(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetHexadecimal(buf, 0x0000, 4);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_STRING_LEN("0000", buf, 4);
}

void test_set_hex_leading_zeros(void) {
    char buf[16] = {0};
    smrtTypeSetHexadecimal(buf, 0x0F, 4);
    TEST_ASSERT_EQUAL_STRING_LEN("000F", buf, 4);
}

//=============================================================================
// Test: smrtTypeCheckHexadecimal
//=============================================================================

void test_check_hex_valid(void) {
    char str[] = "1A2B";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0xFFFF);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0x1A2B, value);
}

void test_check_hex_max_value(void) {
    char str[] = "FFFF";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0xFFFF);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF, value);
}

void test_check_hex_zero(void) {
    char str[] = "0000";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0xFFFF);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0x0000, value);
}

void test_check_hex_exceeds_max(void) {
    char str[] = "FFFF";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0x00FF);
    TEST_ASSERT_FALSE(result);
}

void test_check_hex_invalid_char_lowercase(void) {
    char str[] = "1a2b";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0xFFFF);
    TEST_ASSERT_FALSE(result);
}

void test_check_hex_invalid_char_g(void) {
    char str[] = "1G2B";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 4, 0xFFFF);
    TEST_ASSERT_FALSE(result);
}

void test_check_hex_8_digits(void) {
    char str[] = "DEADBEEF";
    uint32 value = 0;
    bit result = smrtTypeCheckHexadecimal(str, &value, 8, 0xFFFFFFFF);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, value);
}

//=============================================================================
// Test: smrtTypeSetUnsigned
//=============================================================================

void test_set_unsigned_basic(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetUnsigned(buf, 123, 3);
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_STRING_LEN("123", buf, 3);
}

void test_set_unsigned_zero(void) {
    char buf[16] = {0};
    smrtTypeSetUnsigned(buf, 0, 3);
    TEST_ASSERT_EQUAL_STRING_LEN("000", buf, 3);
}

void test_set_unsigned_single_digit(void) {
    char buf[16] = {0};
    smrtTypeSetUnsigned(buf, 5, 1);
    TEST_ASSERT_EQUAL_CHAR('5', buf[0]);
}

void test_set_unsigned_leading_zeros(void) {
    char buf[16] = {0};
    smrtTypeSetUnsigned(buf, 42, 5);
    TEST_ASSERT_EQUAL_STRING_LEN("00042", buf, 5);
}

void test_set_unsigned_max_value(void) {
    char buf[16] = {0};
    smrtTypeSetUnsigned(buf, 255, 3);
    TEST_ASSERT_EQUAL_STRING_LEN("255", buf, 3);
}

//=============================================================================
// Test: smrtTypeCheckUnsigned
//=============================================================================

void test_check_unsigned_valid(void) {
    char str[] = "123";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(123, value);
}

void test_check_unsigned_zero(void) {
    char str[] = "000";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(0, value);
}

void test_check_unsigned_max_boundary(void) {
    char str[] = "255";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 255);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(255, value);
}

void test_check_unsigned_exceeds_max(void) {
    char str[] = "256";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 255);
    TEST_ASSERT_FALSE(result);
}

void test_check_unsigned_invalid_char(void) {
    char str[] = "1A3";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 999);
    TEST_ASSERT_FALSE(result);
}

void test_check_unsigned_space_invalid(void) {
    char str[] = "1 3";
    uint32 value = 0;
    bit result = smrtTypeCheckUnsigned(str, &value, 3, 999);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSetSigned
//=============================================================================

void test_set_signed_positive(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetSigned(buf, 42, 3);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_STRING_LEN("042", &buf[1], 3);
}

void test_set_signed_negative(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetSigned(buf, -42, 3);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_CHAR('-', buf[0]);
    TEST_ASSERT_EQUAL_STRING_LEN("042", &buf[1], 3);
}

void test_set_signed_zero(void) {
    char buf[16] = {0};
    smrtTypeSetSigned(buf, 0, 3);
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_STRING_LEN("000", &buf[1], 3);
}

//=============================================================================
// Test: smrtTypeCheckSigned
//=============================================================================

void test_check_signed_positive(void) {
    char str[] = "+042";
    int32 value = 0;
    bit result = smrtTypeCheckSigned(str, &value, 3, 100, -100);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT32(42, value);
}

void test_check_signed_negative(void) {
    char str[] = "-042";
    int32 value = 0;
    bit result = smrtTypeCheckSigned(str, &value, 3, 100, -100);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT32(-42, value);
}

void test_check_signed_exceeds_max(void) {
    char str[] = "+200";
    int32 value = 0;
    bit result = smrtTypeCheckSigned(str, &value, 3, 100, -100);
    TEST_ASSERT_FALSE(result);
}

void test_check_signed_below_min(void) {
    char str[] = "-200";
    int32 value = 0;
    bit result = smrtTypeCheckSigned(str, &value, 3, 100, -100);
    TEST_ASSERT_FALSE(result);
}

void test_check_signed_no_sign(void) {
    char str[] = "0042";
    int32 value = 0;
    bit result = smrtTypeCheckSigned(str, &value, 3, 100, -100);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSetOnOff
//=============================================================================

void test_set_on(void) {
    char buf[8] = {0};
    uint8 len = smrtTypeSetOnOff(buf, true);
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL_CHAR('O', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('N', buf[1]);
}

void test_set_off(void) {
    char buf[8] = {0};
    uint8 len = smrtTypeSetOnOff(buf, false);
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_CHAR('O', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('F', buf[1]);
    TEST_ASSERT_EQUAL_CHAR('F', buf[2]);
}

//=============================================================================
// Test: smrtTypeCheckOnOff
//=============================================================================

void test_check_on_valid(void) {
    char str[] = "ON";
    uint8 value = 0;
    bit result = smrtTypeCheckOnOff(str, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(true, value);
}

void test_check_off_valid(void) {
    char str[] = "OFF";
    uint8 value = 0;
    bit result = smrtTypeCheckOnOff(str, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(false, value);
}

void test_check_on_off_invalid(void) {
    char str[] = "XY";
    uint8 value = 0;
    bit result = smrtTypeCheckOnOff(str, &value);
    TEST_ASSERT_FALSE(result);
}

void test_check_on_off_partial(void) {
    char str[] = "OX";
    uint8 value = 0;
    bit result = smrtTypeCheckOnOff(str, &value);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSetTrueFalse
//=============================================================================

void test_set_true(void) {
    char buf[8] = {0};
    uint8 len = smrtTypeSetTrueFalse(buf, true);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_STRING_LEN("TRUE", buf, 4);
}

void test_set_false(void) {
    char buf[8] = {0};
    uint8 len = smrtTypeSetTrueFalse(buf, false);
    TEST_ASSERT_EQUAL(5, len);
    TEST_ASSERT_EQUAL_STRING_LEN("FALSE", buf, 5);
}

//=============================================================================
// Test: smrtTypeCheckTrueFalse
//=============================================================================

void test_check_true_valid(void) {
    char str[] = "TRUE";
    uint8 value = 0;
    bit result = smrtTypeCheckTrueFalse(str, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(true, value);
}

void test_check_false_valid(void) {
    char str[] = "FALSE";
    uint8 value = 0;
    bit result = smrtTypeCheckTrueFalse(str, &value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(false, value);
}

void test_check_true_false_invalid(void) {
    char str[] = "MAYBE";
    uint8 value = 0;
    bit result = smrtTypeCheckTrueFalse(str, &value);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSwap16 (function)
//=============================================================================

void test_func_swap16(void) {
    uint8 buf[2] = {0x12, 0x34};
    smrtTypeSwap16(buf);
    TEST_ASSERT_EQUAL_HEX8(0x34, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, buf[1]);
}

void test_func_swap16_same_bytes(void) {
    uint8 buf[2] = {0xAA, 0xAA};
    smrtTypeSwap16(buf);
    TEST_ASSERT_EQUAL_HEX8(0xAA, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, buf[1]);
}

//=============================================================================
// Test: smrtTypeSwap32 (function)
//=============================================================================

void test_func_swap32(void) {
    uint8 buf[4] = {0x12, 0x34, 0x56, 0x78};
    smrtTypeSwap32(buf);
    TEST_ASSERT_EQUAL_HEX8(0x78, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0x56, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0x12, buf[3]);
}

//=============================================================================
// Test: smrtTypeSetUpper / smrtTypeSetLower
//=============================================================================

void test_set_upper(void) {
    char str[] = "hello world";
    smrtTypeSetUpper(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD", str);
}

void test_set_upper_mixed(void) {
    char str[] = "HeLLo 123";
    smrtTypeSetUpper(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("HELLO 123", str);
}

void test_set_lower(void) {
    char str[] = "HELLO WORLD";
    smrtTypeSetLower(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello world", str);
}

void test_set_lower_mixed(void) {
    char str[] = "HeLLo 123";
    smrtTypeSetLower(str, strlen(str));
    TEST_ASSERT_EQUAL_STRING("hello 123", str);
}

//=============================================================================
// Test: smrtTypeSetFixedUnsigned
//=============================================================================

void test_set_fixed_unsigned_basic(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetFixedUnsigned(buf, 12345, 2, 3);
    TEST_ASSERT_EQUAL(6, len);
    TEST_ASSERT_EQUAL_STRING_LEN("12.345", buf, 6);
}

void test_set_fixed_unsigned_zero(void) {
    char buf[16] = {0};
    smrtTypeSetFixedUnsigned(buf, 0, 2, 3);
    TEST_ASSERT_EQUAL_STRING_LEN("00.000", buf, 6);
}

//=============================================================================
// Test: smrtTypeCheckFixedUnsigned
//=============================================================================

void test_check_fixed_unsigned_valid(void) {
    char str[] = "12.345";
    uint32 value = 0;
    bit result = smrtTypeCheckFixedUnsigned(str, &value, 2, 3, 99999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(12345, value);
}

void test_check_fixed_unsigned_zero(void) {
    char str[] = "00.000";
    uint32 value = 0;
    bit result = smrtTypeCheckFixedUnsigned(str, &value, 2, 3, 99999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(0, value);
}

void test_check_fixed_unsigned_invalid_no_dot(void) {
    char str[] = "123456";
    uint32 value = 0;
    bit result = smrtTypeCheckFixedUnsigned(str, &value, 2, 3, 99999);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSetFixedSigned
//=============================================================================

void test_set_fixed_signed_positive(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetFixedSigned(buf, 12345, 2, 3);
    TEST_ASSERT_EQUAL(7, len);
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_STRING_LEN("12.345", &buf[1], 6);
}

void test_set_fixed_signed_negative(void) {
    char buf[16] = {0};
    uint8 len = smrtTypeSetFixedSigned(buf, -12345, 2, 3);
    TEST_ASSERT_EQUAL(7, len);
    TEST_ASSERT_EQUAL_CHAR('-', buf[0]);
    TEST_ASSERT_EQUAL_STRING_LEN("12.345", &buf[1], 6);
}

//=============================================================================
// Test: smrtTypeCheckFixedSigned
//=============================================================================

void test_check_fixed_signed_positive(void) {
    char str[] = "+12.345";
    int32 value = 0;
    bit result = smrtTypeCheckFixedSigned(str, &value, 2, 3, 99999, -99999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT32(12345, value);
}

void test_check_fixed_signed_negative(void) {
    char str[] = "-12.345";
    int32 value = 0;
    bit result = smrtTypeCheckFixedSigned(str, &value, 2, 3, 99999, -99999);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT32(-12345, value);
}

void test_check_fixed_signed_no_sign(void) {
    char str[] = "012.345";
    int32 value = 0;
    bit result = smrtTypeCheckFixedSigned(str, &value, 2, 3, 99999, -99999);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeSetIP
//=============================================================================

void test_set_ip_basic(void) {
    char buf[20] = {0};
    uint8 len = smrtTypeSetIP(buf, 0xC0A80164);
    TEST_ASSERT_EQUAL(16, len);
    TEST_ASSERT_EQUAL_STRING_LEN("192.168.001.100.", buf, 16);
}

void test_set_ip_zeros(void) {
    char buf[20] = {0};
    smrtTypeSetIP(buf, 0x00000000);
    TEST_ASSERT_EQUAL_STRING_LEN("000.000.000.000.", buf, 16);
}

//=============================================================================
// Test: smrtTypeCheckIP
//=============================================================================

void test_check_ip_valid(void) {
    char str[] = "192.168.001.100.";
    uint32 addr = 0;
    bit result = smrtTypeCheckIP(str, &addr);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0xC0A80164, addr);
}

void test_check_ip_zeros(void) {
    char str[] = "000.000.000.000.";
    uint32 addr = 0;
    bit result = smrtTypeCheckIP(str, &addr);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, addr);
}

void test_check_ip_invalid_octet(void) {
    char str[] = "999.168.001.100.";
    uint32 addr = 0;
    bit result = smrtTypeCheckIP(str, &addr);
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Test: smrtTypeMemFill16
//=============================================================================

void test_memfill16_basic(void) {
    uint16 buf[4] = {0};
    smrtTypeMemFill16(buf, 0xABCD, 4);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_HEX16(0xABCD, buf[i]);
    }
}

void test_memfill16_zero_count(void) {
    uint16 buf[4] = {0x1111, 0x2222, 0x3333, 0x4444};
    smrtTypeMemFill16(buf, 0xABCD, 0);
    TEST_ASSERT_EQUAL_HEX16(0x1111, buf[0]);
    TEST_ASSERT_EQUAL_HEX16(0x2222, buf[1]);
}

//=============================================================================
// Test: smrtTypeMemFill32
//=============================================================================

void test_memfill32_basic(void) {
    uint32 buf[4] = {0};
    smrtTypeMemFill32(buf, 0xDEADBEEF, 4);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, buf[i]);
    }
}

//=============================================================================
// Test: smrtTypeStrCpy
//=============================================================================

void test_strcpy_basic(void) {
    char src[] = "Hello";
    char dst[16] = {0};
    uint32 len = smrtTypeStrCpy(dst, src);
    TEST_ASSERT_EQUAL_UINT32(5, len);
    TEST_ASSERT_EQUAL_STRING("Hello", dst);
}

void test_strcpy_empty(void) {
    char src[] = "";
    char dst[16] = {'X', 'X', 'X', 0};
    uint32 len = smrtTypeStrCpy(dst, src);
    TEST_ASSERT_EQUAL_UINT32(0, len);
    TEST_ASSERT_EQUAL_STRING("", dst);
}

//=============================================================================
// Test: smrtTypeStrnCpy
//=============================================================================

void test_strncpy_basic(void) {
    char src[] = "Hello";
    char dst[16] = {0};
    uint32 len = smrtTypeStrnCpy(dst, src, 16);
    TEST_ASSERT_EQUAL_UINT32(5, len);
    TEST_ASSERT_EQUAL_STRING("Hello", dst);
}

void test_strncpy_truncated(void) {
    char src[] = "Hello World";
    char dst[16] = {0};
    uint32 len = smrtTypeStrnCpy(dst, src, 5);
    TEST_ASSERT_EQUAL_UINT32(5, len);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", dst, 5);
}

void test_strncpy_exact_fit(void) {
    char src[] = "ABC";
    char dst[16] = {0};
    uint32 len = smrtTypeStrnCpy(dst, src, 3);
    TEST_ASSERT_EQUAL_UINT32(3, len);
    TEST_ASSERT_EQUAL_STRING_LEN("ABC", dst, 3);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Macro tests - SWAP
    RUN_TEST(test_macro_swap16_basic);
    RUN_TEST(test_macro_swap16_zero);
    RUN_TEST(test_macro_swap16_ff);
    RUN_TEST(test_macro_swap32_basic);
    RUN_TEST(test_macro_swap32_zero);

    // Macro tests - TO_UPPER / TO_LOWER
    RUN_TEST(test_macro_to_upper_lowercase);
    RUN_TEST(test_macro_to_upper_already_upper);
    RUN_TEST(test_macro_to_upper_non_alpha);
    RUN_TEST(test_macro_to_lower_uppercase);
    RUN_TEST(test_macro_to_lower_already_lower);

    // Macro tests - MIN / MAX
    RUN_TEST(test_macro_min);
    RUN_TEST(test_macro_max);

    // Hexadecimal
    RUN_TEST(test_set_hex_single_digit);
    RUN_TEST(test_set_hex_two_digits);
    RUN_TEST(test_set_hex_four_digits);
    RUN_TEST(test_set_hex_eight_digits);
    RUN_TEST(test_set_hex_zero);
    RUN_TEST(test_set_hex_leading_zeros);
    RUN_TEST(test_check_hex_valid);
    RUN_TEST(test_check_hex_max_value);
    RUN_TEST(test_check_hex_zero);
    RUN_TEST(test_check_hex_exceeds_max);
    RUN_TEST(test_check_hex_invalid_char_lowercase);
    RUN_TEST(test_check_hex_invalid_char_g);
    RUN_TEST(test_check_hex_8_digits);

    // Unsigned decimal
    RUN_TEST(test_set_unsigned_basic);
    RUN_TEST(test_set_unsigned_zero);
    RUN_TEST(test_set_unsigned_single_digit);
    RUN_TEST(test_set_unsigned_leading_zeros);
    RUN_TEST(test_set_unsigned_max_value);
    RUN_TEST(test_check_unsigned_valid);
    RUN_TEST(test_check_unsigned_zero);
    RUN_TEST(test_check_unsigned_max_boundary);
    RUN_TEST(test_check_unsigned_exceeds_max);
    RUN_TEST(test_check_unsigned_invalid_char);
    RUN_TEST(test_check_unsigned_space_invalid);

    // Signed decimal
    RUN_TEST(test_set_signed_positive);
    RUN_TEST(test_set_signed_negative);
    RUN_TEST(test_set_signed_zero);
    RUN_TEST(test_check_signed_positive);
    RUN_TEST(test_check_signed_negative);
    RUN_TEST(test_check_signed_exceeds_max);
    RUN_TEST(test_check_signed_below_min);
    RUN_TEST(test_check_signed_no_sign);

    // ON/OFF
    RUN_TEST(test_set_on);
    RUN_TEST(test_set_off);
    RUN_TEST(test_check_on_valid);
    RUN_TEST(test_check_off_valid);
    RUN_TEST(test_check_on_off_invalid);
    RUN_TEST(test_check_on_off_partial);

    // TRUE/FALSE
    RUN_TEST(test_set_true);
    RUN_TEST(test_set_false);
    RUN_TEST(test_check_true_valid);
    RUN_TEST(test_check_false_valid);
    RUN_TEST(test_check_true_false_invalid);

    // Swap functions
    RUN_TEST(test_func_swap16);
    RUN_TEST(test_func_swap16_same_bytes);
    RUN_TEST(test_func_swap32);

    // Upper/Lower functions
    RUN_TEST(test_set_upper);
    RUN_TEST(test_set_upper_mixed);
    RUN_TEST(test_set_lower);
    RUN_TEST(test_set_lower_mixed);

    // Fixed-point unsigned
    RUN_TEST(test_set_fixed_unsigned_basic);
    RUN_TEST(test_set_fixed_unsigned_zero);
    RUN_TEST(test_check_fixed_unsigned_valid);
    RUN_TEST(test_check_fixed_unsigned_zero);
    RUN_TEST(test_check_fixed_unsigned_invalid_no_dot);

    // Fixed-point signed
    RUN_TEST(test_set_fixed_signed_positive);
    RUN_TEST(test_set_fixed_signed_negative);
    RUN_TEST(test_check_fixed_signed_positive);
    RUN_TEST(test_check_fixed_signed_negative);
    RUN_TEST(test_check_fixed_signed_no_sign);

    // IP address
    RUN_TEST(test_set_ip_basic);
    RUN_TEST(test_set_ip_zeros);
    RUN_TEST(test_check_ip_valid);
    RUN_TEST(test_check_ip_zeros);
    RUN_TEST(test_check_ip_invalid_octet);

    // Memory fill
    RUN_TEST(test_memfill16_basic);
    RUN_TEST(test_memfill16_zero_count);
    RUN_TEST(test_memfill32_basic);

    // String copy
    RUN_TEST(test_strcpy_basic);
    RUN_TEST(test_strcpy_empty);
    RUN_TEST(test_strncpy_basic);
    RUN_TEST(test_strncpy_truncated);
    RUN_TEST(test_strncpy_exact_fit);

    return UNITY_END();
}
