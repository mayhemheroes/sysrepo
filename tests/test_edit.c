
#define _GNU_SOURCE

#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>
#include <libyang/libyang.h>

#include "tests/config.h"
#include "sysrepo.h"

struct state {
    sr_conn_ctx_t *conn;
    sr_session_ctx_t *sess;
};

static int
setup_f(void **state)
{
    struct state *st;

    st = malloc(sizeof *st);
    if (!st) {
        return 1;
    }
    *state = st;

    if (sr_connect("test1", 0, &st->conn) != SR_ERR_OK) {
        return 1;
    }

    if (sr_install_module(st->conn, TESTS_DIR "/files/test.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/ietf-interfaces.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/iana-if-type.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }

    if (sr_session_start(st->conn, SR_DS_RUNNING, 0, &st->sess) != SR_ERR_OK) {
        return 1;
    }

    return 0;
}

static int
teardown_f(void **state)
{
    struct state *st = (struct state *)*state;

    sr_remove_module(st->conn, "ietf-interfaces");
    sr_remove_module(st->conn, "iana-if-type");
    sr_remove_module(st->conn, "test");

    sr_disconnect(st->conn);
    free(st);
    return 0;
}

static int
clear_interfaces(void **state)
{
    struct state *st = (struct state *)*state;

    sr_delete_item(st->sess, "/ietf-interfaces:interfaces", 0);
    sr_apply_changes(st->sess);

    return 0;
}

static int
clear_test(void **state)
{
    struct state *st = (struct state *)*state;

    sr_delete_item(st->sess, "/test:l1[k='key1']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:l1[k='key2']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:l1[k='key3']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:ll1[.='-1']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:ll1[.='-2']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:ll1[.='-3']", SR_EDIT_STRICT);
    sr_delete_item(st->sess, "/test:cont", 0);
    sr_apply_changes(st->sess);

    return 0;
}

static void
test_delete(void **state)
{
    struct state *st = (struct state *)*state;
    struct lyd_node *subtree;
    char *str;
    int ret;

    /* remove on no data */
    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* delete on no data */
    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type", SR_EDIT_STRICT);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_NOT_FOUND);
    ret = sr_discard_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* delete a leaf without exact value */
    ret = sr_set_item_str(st->sess, "/test:test-leaf", "16", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_delete_item(st->sess, "/test:test-leaf", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* check final datastore contents */
    ret = sr_get_subtree(st->sess, "/ietf-interfaces:interfaces", &subtree);
    assert_int_equal(ret, SR_ERR_OK);

    lyd_print_mem(&str, subtree, LYD_XML, LYP_WITHSIBLINGS);
    assert_null(str);
    lyd_free(subtree);
}

static void
test_delete_invalid(void **state)
{
    struct state *st = (struct state *)*state;
    int ret;

    /* no keys */
    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface/type", 0);
    assert_int_equal(ret, SR_ERR_INVAL_ARG);
    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface", 0);
    assert_int_equal(ret, SR_ERR_INVAL_ARG);
    ret = sr_delete_item(st->sess, "/test:l1", 0);
    assert_int_equal(ret, SR_ERR_INVAL_ARG);

    /* no leaf-list value */
    ret = sr_delete_item(st->sess, "/test:ll1", 0);
    assert_int_equal(ret, SR_ERR_INVAL_ARG);
}

static void
test_create1(void **state)
{
    struct state *st = (struct state *)*state;
    struct lyd_node *subtree;
    char *str;
    const char *str2;
    int ret;

    /* one-by-one create */
    ret = sr_set_item_str(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']", NULL, SR_EDIT_STRICT);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type",
            "iana-if-type:ethernetCsmacd", SR_EDIT_STRICT);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_get_subtree(st->sess, "/ietf-interfaces:interfaces", &subtree);
    assert_int_equal(ret, SR_ERR_OK);

    lyd_print_mem(&str, subtree, LYD_XML, LYP_WITHSIBLINGS);
    lyd_free(subtree);

    str2 =
    "<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
        "<interface>"
            "<name>eth64</name>"
            "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
        "</interface>"
    "</interfaces>";

    assert_string_equal(str, str2);
    free(str);

    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* create with non-existing parents */
    ret = sr_set_item_str(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type",
            "iana-if-type:ethernetCsmacd", SR_EDIT_NON_RECURSIVE);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_NOT_FOUND);
    ret = sr_discard_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);
}

static void
test_create2(void **state)
{
    struct state *st = (struct state *)*state;
    struct lyd_node *subtree;
    char *str;
    const char *str2;
    int ret;

    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type",
            "iana-if-type:ethernetCsmacd", SR_EDIT_STRICT);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_UNSUPPORTED);
    ret = sr_discard_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces/interface[name='eth68']", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/ietf-interfaces:interfaces/interface[name='eth64']/type",
            "iana-if-type:ethernetCsmacd", SR_EDIT_STRICT);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_get_subtree(st->sess, "/ietf-interfaces:interfaces", &subtree);
    assert_int_equal(ret, SR_ERR_OK);

    lyd_print_mem(&str, subtree, LYD_XML, LYP_WITHSIBLINGS);
    lyd_free(subtree);

    str2 =
    "<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
        "<interface>"
            "<name>eth64</name>"
            "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
        "</interface>"
    "</interfaces>";

    assert_string_equal(str, str2);
    free(str);

    ret = sr_delete_item(st->sess, "/ietf-interfaces:interfaces", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_get_subtree(st->sess, "/ietf-interfaces:interfaces", &subtree);
    assert_int_equal(ret, SR_ERR_OK);

    lyd_print_mem(&str, subtree, LYD_XML, LYP_WITHSIBLINGS);
    assert_null(str);
    lyd_free(subtree);
}

static void
test_move1(void **state)
{
    struct state *st = (struct state *)*state;
    struct ly_set *subtrees;
    char *str, *str2;
    uint32_t i;
    int ret;

    /* create top-level testing data */
    ret = sr_set_item_str(st->sess, "/test:l1[k='key1']/v", "1", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:l1[k='key2']/v", "2", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:l1[k='key3']/v", "3", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:ll1", "-1", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:ll1", "-2", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:ll1", "-3", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* perform some move operations */
    ret = sr_move_item(st->sess, "/test:l1[k='key3']", SR_MOVE_FIRST, NULL, NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:l1[k='key1']", SR_MOVE_AFTER, "[k='key2']", NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:ll1[.='-3']", SR_MOVE_FIRST, NULL, NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:ll1[.='-1']", SR_MOVE_AFTER, NULL, "-2");
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_get_subtrees(st->sess, "/test:*", &subtrees);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(subtrees->number, 7);

    /* should be in reversed order (relative only to the same schema node instances) */
    for (i = 0; i < 7; ++i) {
        lyd_print_mem(&str, subtrees->set.d[i], LYD_XML, 0);

        switch (i) {
        case 0:
            asprintf(&str2, "<ll1 xmlns=\"urn:test\">-%u</ll1>", 3);
            break;
        case 1:
            asprintf(&str2,
            "<l1 xmlns=\"urn:test\">"
                "<k>key%u</k>"
                "<v>%u</v>"
            "</l1>", 3, 3);
            break;
        case 2:
            assert_null(str);
            lyd_free(subtrees->set.d[i]);
            continue;
        case 3:
            asprintf(&str2,
            "<l1 xmlns=\"urn:test\">"
                "<k>key%u</k>"
                "<v>%u</v>"
            "</l1>", 2, 2);
            break;
        case 4:
            asprintf(&str2,
            "<l1 xmlns=\"urn:test\">"
                "<k>key%u</k>"
                "<v>%u</v>"
            "</l1>", 1, 1);
            break;
        case 5:
            asprintf(&str2, "<ll1 xmlns=\"urn:test\">-%u</ll1>", 2);
            break;
        case 6:
            asprintf(&str2, "<ll1 xmlns=\"urn:test\">-%u</ll1>", 1);
            break;
        default:
            fail();
        }

        assert_string_equal(str, str2);
        free(str2);
        free(str);

        lyd_free(subtrees->set.d[i]);
    }
    ly_set_free(subtrees);

    /* create nested testing data */
    ret = sr_set_item_str(st->sess, "/test:cont/l2[k='key1']/v", "1", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:cont/l2[k='key2']/v", "2", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:cont/l2[k='key3']/v", "3", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:cont/ll2", "-1", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:cont/ll2", "-2", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_set_item_str(st->sess, "/test:cont/ll2", "-3", 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* perform some move operations */
    ret = sr_move_item(st->sess, "/test:cont/l2[k='key1']", SR_MOVE_LAST, NULL, NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:cont/l2[k='key3']", SR_MOVE_BEFORE, "[k='key2']", NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:cont/ll2[.='-1']", SR_MOVE_LAST, NULL, NULL);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_move_item(st->sess, "/test:cont/ll2[.='-3']", SR_MOVE_BEFORE, NULL, "-2");
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_apply_changes(st->sess);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_get_subtrees(st->sess, "/test:cont", &subtrees);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(subtrees->number, 1);

    /* should be in reversed order (relative only to the same schema node instances) */
    lyd_print_mem(&str, subtrees->set.d[0], LYD_XML, 0);

    str2 =
    "<cont xmlns=\"urn:test\">"
        "<l2><k>key3</k><v>3</v></l2>"
        "<l2><k>key2</k><v>2</v></l2>"
        "<ll2>-3</ll2>"
        "<ll2>-2</ll2>"
        "<l2><k>key1</k><v>1</v></l2>"
        "<ll2>-1</ll2>"
    "</cont>";
    assert_string_equal(str, str2);

    free(str);
    lyd_free(subtrees->set.d[0]);
    ly_set_free(subtrees);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_teardown(test_delete, clear_interfaces),
        cmocka_unit_test(test_delete_invalid),
        cmocka_unit_test_teardown(test_create1, clear_interfaces),
        cmocka_unit_test_teardown(test_create2, clear_interfaces),
        cmocka_unit_test_teardown(test_move1, clear_test),
    };

    return cmocka_run_group_tests(tests, setup_f, teardown_f);
}