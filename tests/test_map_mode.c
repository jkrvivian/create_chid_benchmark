/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include <time.h>
#include "map/mode.h"
#include "test_define.h"
#include "cclient/api/core/core_api.h"
#include "cclient/api/extended/extended_api.h"
#include "common/trinary/trit_tryte.h"
#include "common/trinary/tryte_ascii.h"

#define CHID "UANFAVTSAXZMYUWRECNAOJDAQVTTORVGJCCISMZYAFFU9EYLBMZKEJ9VNXVFFGUTCHONEYVWVUTBTDJLO"
#define NEW_CHID "ONMTPDICUWBGEGODWKGBGMLNAZFXNHCJITSSTBTGMXCXBXJFBPOPXFPOJTXKOOSAJOZAYANZZBFKYHJ9N"
#define EPID "KI99YKKLFALYRUVRXKKRJCPVFISPMNCQQSMB9BGUWIHZTYFQOBZWYSVRNKVFJLSPPLPSFNBNREJWOR99U"

struct timespec start_time, end_time;
double diff_time(struct timespec start, struct timespec end) {
  struct timespec diff;
  if (end.tv_nsec - start.tv_nsec < 0) {
    diff.tv_sec = end.tv_sec - start.tv_sec - 1;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
  } else {
    diff.tv_sec = end.tv_sec - start.tv_sec;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

void test_time_start(struct timespec* start) { clock_gettime(CLOCK_REALTIME, start); }

void test_time_end(struct timespec* start, struct timespec* end, double* sum) {
  clock_gettime(CLOCK_REALTIME, end);
  double difference = diff_time(*start, *end);
  printf("%lf\n", difference);
  *sum += difference;
}

retcode_t send_bundle(bundle_transactions_t *const bundle) {
  retcode_t ret = RC_OK;
  iota_client_service_t serv;
  serv.http.path = "/";
  serv.http.content_type = "application/json";
  serv.http.accept = "application/json";
  serv.http.host = "localhost";
  serv.http.port = 14265;
  serv.http.api_version = 1;
  serv.serializer_type = SR_JSON;
  serv.http.ca_pem = NULL;
  iota_client_core_init(&serv);
  iota_client_extended_init();

  Kerl kerl;
  kerl_init(&kerl);
  bundle_finalize(bundle, &kerl);
  transaction_array_t *out_tx_objs = transaction_array_new();
  hash8019_array_p raw_trytes = hash8019_array_new();
  iota_transaction_t *curr_tx = NULL;
  flex_trit_t trits_8019[FLEX_TRIT_SIZE_8019];

  BUNDLE_FOREACH(bundle, curr_tx) {
    transaction_serialize_on_flex_trits(curr_tx, trits_8019);
    hash_array_push(raw_trytes, trits_8019);
  }
  if ((ret = iota_client_send_trytes(&serv, raw_trytes, 1, 14, NULL, true, out_tx_objs)) != RC_OK) {
    goto done;
  }

done:
  hash_array_free(raw_trytes);
  transaction_array_free(out_tx_objs);
  iota_client_extended_destroy();
  iota_client_core_destroy(&serv);

  return ret;
}

void test_channel_create(void) {
  for(int i = 1; i <= 7; ++i) {
      double sum = 0;
      for (int j = 0; j < 100; ++j) {
          mam_api_t mam;
          tryte_t channel_id[MAM_CHANNEL_ID_TRYTE_SIZE + 1];
          mam_api_init(&mam, (tryte_t *)TRYTES_81_1);

        test_time_start(&start_time);
          map_channel_create(&mam, channel_id, i);
        test_time_end(&start_time, &end_time, &sum);
          channel_id[MAM_CHANNEL_ID_TRYTE_SIZE] = '\0';
          //TEST_ASSERT_EQUAL_STRING(CHID, channel_id);

          mam_api_destroy(&mam);
      }
      printf("Average time of send_mam_message: %lf\n", sum / 100.0);
  }
}

void test_announce_channel(void) {
  mam_api_t mam;
  bundle_transactions_t *bundle = NULL;
  bundle_transactions_new(&bundle);
  tryte_t channel_id[MAM_CHANNEL_ID_TRYTE_SIZE + 1];
  trit_t msg_id[MAM_MSG_ID_SIZE];
  mam_api_init(&mam, (tryte_t *)TRYTES_81_1);

  map_channel_create(&mam, channel_id, 1);
  map_announce_channel(&mam, channel_id, bundle, msg_id, channel_id);
  channel_id[MAM_CHANNEL_ID_TRYTE_SIZE] = '\0';
  TEST_ASSERT_EQUAL_STRING(NEW_CHID, channel_id);

  bundle_transactions_free(&bundle);
  mam_api_destroy(&mam);
}

void test_announce_endpoint(void) {
  mam_api_t mam;
  bundle_transactions_t *bundle = NULL;
  bundle_transactions_new(&bundle);
  tryte_t channel_id[MAM_CHANNEL_ID_TRYTE_SIZE + 1];
  trit_t msg_id[MAM_MSG_ID_SIZE];
  mam_api_init(&mam, (tryte_t *)TRYTES_81_1);

  map_channel_create(&mam, channel_id, 1);
  // Channel_id is actually the new endpoint id
  map_announce_endpoint(&mam, channel_id, bundle, msg_id, channel_id);
  channel_id[MAM_CHANNEL_ID_TRYTE_SIZE] = '\0';
  TEST_ASSERT_EQUAL_STRING(EPID, channel_id);

  bundle_transactions_free(&bundle);
  mam_api_destroy(&mam);
}

void test_write_message(void) {
  double sum = 0;
  int num_round = 100;
  for (int i = 0; i < num_round; ++i) {
      mam_api_t mam;
      tryte_t channel_id[MAM_CHANNEL_ID_TRYTE_SIZE + 1];
      trit_t msg_id[MAM_MSG_ID_SIZE];
      mam_api_init(&mam, (tryte_t *)TRYTES_81_1);
      retcode_t ret = RC_ERROR;

      map_channel_create(&mam, channel_id, 2);
      printf("%s", (char *)channel_id);
      for (int j = 1; j <= 4; ++j) {
      test_time_start(&start_time);
          bundle_transactions_t *bundle = NULL;
          bundle_transactions_new(&bundle);
          ret = map_write_header_on_channel(&mam, channel_id, bundle, msg_id);
          TEST_ASSERT_EQUAL(RC_OK, ret);

          if (j == 4) {
              ret = map_write_packet(&mam, bundle, TEST_PAYLOAD, msg_id, true);
          } else {
              ret = map_write_packet(&mam, bundle, TEST_PAYLOAD, msg_id, false);
          }
          TEST_ASSERT_EQUAL(RC_OK, ret);
          ret = send_bundle(bundle);
          TEST_ASSERT_EQUAL(RC_OK, ret);
      test_time_end(&start_time, &end_time, &sum);

          bundle_transactions_free(&bundle);
      }
      mam_api_destroy(&mam);
  }
  printf("Average time of send_mam_message: %lf\n", sum / num_round);
}

void test_bundle_read(void) {
  retcode_t ret;
  char *payload = NULL;
  mam_api_t mam;
  bundle_transactions_t *bundle = NULL;
  bundle_transactions_new(&bundle);

  ret = mam_api_init(&mam, (tryte_t *)SEED);
  TEST_ASSERT_EQUAL(RC_OK, ret);

  flex_trit_t chid_trits[NUM_TRITS_HASH];
  flex_trits_from_trytes(chid_trits, NUM_TRITS_HASH, (const tryte_t *)CHID_BUNDLE, NUM_TRYTES_HASH, NUM_TRYTES_HASH);
  mam_api_add_trusted_channel_pk(&mam, chid_trits);

  flex_trit_t hash[NUM_TRITS_SERIALIZED_TRANSACTION];
  flex_trits_from_trytes(hash, NUM_TRITS_SERIALIZED_TRANSACTION, (tryte_t const *)TEST_MAM_TRANSACTION_TRYTES_1,
                         NUM_TRYTES_SERIALIZED_TRANSACTION, NUM_TRYTES_SERIALIZED_TRANSACTION);
  iota_transaction_t *txn = transaction_deserialize(hash, false);
  bundle_transactions_add(bundle, txn);
  transaction_free(txn);

  flex_trits_from_trytes(hash, NUM_TRITS_SERIALIZED_TRANSACTION, (tryte_t const *)TEST_MAM_TRANSACTION_TRYTES_2,
                         NUM_TRYTES_SERIALIZED_TRANSACTION, NUM_TRYTES_SERIALIZED_TRANSACTION);
  txn = transaction_deserialize(hash, false);
  bundle_transactions_add(bundle, txn);
  transaction_free(txn);

  flex_trits_from_trytes(hash, NUM_TRITS_SERIALIZED_TRANSACTION, (tryte_t const *)TEST_MAM_TRANSACTION_TRYTES_3,
                         NUM_TRYTES_SERIALIZED_TRANSACTION, NUM_TRYTES_SERIALIZED_TRANSACTION);
  txn = transaction_deserialize(hash, false);
  bundle_transactions_add(bundle, txn);

  ret = map_api_bundle_read(&mam, bundle, &payload);
  TEST_ASSERT_EQUAL(RC_OK, ret);

  TEST_ASSERT_EQUAL_STRING(TEST_PAYLOAD, payload);
  transaction_free(txn);
  bundle_transactions_free(&bundle);
  mam_api_destroy(&mam);
  free(payload);
}

int main(void) {
  UNITY_BEGIN();
  /*
  RUN_TEST(test_channel_create);
  RUN_TEST(test_announce_channel);
  RUN_TEST(test_announce_endpoint);
  */
  RUN_TEST(test_write_message);
  /*
  RUN_TEST(test_bundle_read);
  */
  return UNITY_END();
}
