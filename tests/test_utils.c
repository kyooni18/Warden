/*
 * tests/test_utils.c — Minimal unit tests for the engine utilities module.
 *
 * Compiled without Feather or ncurses; links only warden_engine.
 * Add new test functions following the PASS/FAIL pattern below.
 */

#include "engine/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Simple test harness ---- */

static int g_pass = 0;
static int g_fail = 0;

#define EXPECT_EQ(a, b) do { \
    if ((a) == (b)) { \
        printf("  PASS: %s == %s\n", #a, #b); \
        g_pass++; \
    } else { \
        printf("  FAIL: %s == %s  (got %d, expected %d) at %s:%d\n", \
               #a, #b, (int)(a), (int)(b), __FILE__, __LINE__); \
        g_fail++; \
    } \
} while (0)

#define EXPECT_TRUE(expr) do { \
    if (expr) { \
        printf("  PASS: %s\n", #expr); \
        g_pass++; \
    } else { \
        printf("  FAIL: %s (expected true) at %s:%d\n", #expr, __FILE__, __LINE__); \
        g_fail++; \
    } \
} while (0)

#define EXPECT_FALSE(expr) do { \
    if (!(expr)) { \
        printf("  PASS: !(%s)\n", #expr); \
        g_pass++; \
    } else { \
        printf("  FAIL: !(%s) (expected false) at %s:%d\n", #expr, __FILE__, __LINE__); \
        g_fail++; \
    } \
} while (0)

#define EXPECT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) == 0) { \
        printf("  PASS: strcmp(\"%s\", \"%s\") == 0\n", (a), (b)); \
        g_pass++; \
    } else { \
        printf("  FAIL: expected \"%s\", got \"%s\" at %s:%d\n", \
               (b), (a), __FILE__, __LINE__); \
        g_fail++; \
    } \
} while (0)

/* ---- Test functions ---- */

static void test_clamp_int(void)
{
    printf("clamp_int:\n");
    EXPECT_EQ(clamp_int(5,  0, 10),  5);
    EXPECT_EQ(clamp_int(-1, 0, 10),  0);
    EXPECT_EQ(clamp_int(11, 0, 10), 10);
    EXPECT_EQ(clamp_int(0,  0,  0),  0);
    EXPECT_EQ(clamp_int(0,  5, 10),  5);
}

static void test_roll_range(void)
{
    int i;
    printf("roll_range:\n");
    /* All results must be within bounds over many trials. */
    srand(42);
    for (i = 0; i < 1000; i++) {
        int r = roll_range(3, 7);
        EXPECT_TRUE(r >= 3 && r <= 7);
        if (r < 3 || r > 7) break; /* stop spamming on failure */
    }
    EXPECT_EQ(roll_range(5, 5), 5); /* degenerate range */
}

static void test_is_blank(void)
{
    printf("is_blank:\n");
    EXPECT_TRUE(is_blank(""));
    EXPECT_TRUE(is_blank("   "));
    EXPECT_TRUE(is_blank("\t\n\r"));
    EXPECT_FALSE(is_blank("hello"));
    EXPECT_FALSE(is_blank("  x  "));
}

static void test_canonicalize_input(void)
{
    char buf[64];
    printf("canonicalize_input:\n");

    canonicalize_input("Hello World", buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "hello world");

    canonicalize_input("  NORTH  ", buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "north");

    canonicalize_input("use  potion", buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "use potion");

    canonicalize_input("", buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "");

    canonicalize_input("   ", buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "");
}

static void test_starts_with(void)
{
    printf("starts_with:\n");
    EXPECT_TRUE(starts_with("hello world", "hello"));
    EXPECT_TRUE(starts_with("north", "north"));
    EXPECT_TRUE(starts_with("abc", ""));
    EXPECT_FALSE(starts_with("hello", "world"));
    EXPECT_FALSE(starts_with("", "x"));
}

static void test_minutes_to_ms(void)
{
    printf("minutes_to_ms:\n");
    EXPECT_EQ((int)minutes_to_ms(0),   0);
    EXPECT_EQ((int)minutes_to_ms(1),   1000);
    EXPECT_EQ((int)minutes_to_ms(60),  60000);
    EXPECT_EQ((int)minutes_to_ms(180), 180000);
}

/* ---- Entry point ---- */

int main(void)
{
    printf("=== Warden Engine Utils Tests ===\n\n");

    test_clamp_int();
    test_roll_range();
    test_is_blank();
    test_canonicalize_input();
    test_starts_with();
    test_minutes_to_ms();

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
