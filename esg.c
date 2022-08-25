/**
 * This hook accumulates xrp until it reaches the amount defined in this script.
 * When this trigger occurs the hook finds the open offers for CRC on the defined account
 * and places buy offer/s to empty the balance.
 */

// 1. Acumulate XRP
// 2. Trigger at $
// 3. Create Buy Offer/s on CRC

// Exceptions: Buy not accepted. Buy accepted dne (no longer exists)

#define HAS_CALLBACK
#include <stdint.h>
#include "hookapi.h"

#define ACCT_LN "John Doe"
#define ACCT_ADDRESS "rfESGVNTuXckX6x2qTMFmFSnm6dEWGX"
#define CARBONLAND_ADDRESS "rnDdYnvVSbzXbCZEeJaciXbcpafbYraxb4"
#define SCOPE_ONE 0.60f
#define SCOPE_TWO 0.30f
#define SCOPE_THREE 0.10f
#define TRIGGER_AMOUNT 10500000000

int64_t cbak(uint32_t reserved)
{
    TRACESTR("ESG: callback called.");
    return 0;
}

int64_t hook(uint32_t reserved)
{

    TRACESTR("ESG: started");

    // before we start calling hook-api functions we should tell the hook how many tx we intend to create
    // etxn_reserve(1); // we are going to emit 1 transaction

    // this api fetches the AccountID of the account the hook currently executing is installed on
    // since hooks can be triggered by both incoming and ougoing transactions this is important to know
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // next fetch the sfAccount field from the originating transaction
    uint8_t account_field[20];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);
    TRACEVAR(account_field_len);
    if (account_field_len < 20)                                   // negative values indicate errors from every api
        rollback(SBUF("ESG: sfAccount field missing!!!"), 1);  // this code could never be hit in prod
                                                                  // but it's here for completeness

    // compare the "From Account" (sfAccount) on the transaction with the account the hook is running on
    int equal = 0; BUFFER_EQUAL(equal, hook_accid, account_field, 20);
    if (equal)
    {
        // if the accounts are equal (memcmp == 0) the otxn was sent from the hook account by someone else
        // accept() it and end the hook execution here
        accept(SBUF("ESG: Outgoing transaction"), 2);
    }

    unsigned char amount_buffer[48];
    int64_t amount_len = otxn_field(SBUF(amount_buffer), sfAmount);
    int64_t sell_drops_raw = TRIGGER_AMOUNT;
    if (amount_len != 8)
    {
        accept(SBUF("ESG: Non-XRP accepting"), 2);
    }
    TRACESTR("ESG: XRP transaction detected, calculating");

    uint8_t keylet[34];
    if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, hook_accid, 20, 0, 0, 0, 0) != 34)
        rollback(SBUF("MintedTokens: Internal error, could not generate keylet"), 10);

    // then requesting XRPLD slot that keylet into a new slot for us
    int64_t slot_no = slot_set(SBUF(keylet), 0);
    if (slot_no < 0)
    rollback(SBUF("MintedTokens: Could not set keylet in slot"), 10);

    int64_t seq_slot = slot_subfield(slot_no, sfBalance, 0);
    if (seq_slot < 0)
        rollback(SBUF("ESG: Could not find sfBalance on hook account"), 20);

    uint8_t seq_buf[48];
    seq_slot = slot(SBUF(seq_buf), seq_slot);
    if (seq_slot != 8)
        rollback(SBUF("ESG: Could not fetch sequence from sfBalance."), 80);
    
    // // then conver the four byte buffer to an unsigned 32 bit integer
    int64_t balance_drops = AMOUNT_TO_DROPS(seq_buf);
    TRACEVAR(balance_drops); // print the integer for debugging purposes

    int64_t otxn_drops = AMOUNT_TO_DROPS(amount_buffer);
    TRACEVAR(otxn_drops);
    
    if (TRIGGER_AMOUNT == (balance_drops + otxn_drops) 
    {
        // Buy & Retire Carbon Removal Credit
        // now check if the sender is on the signer list
        // we can do this by first creating a keylet that describes the signer list on the hook account
        uint8_t keylet[34];
        CLEARBUF(keylet);
        if (util_keylet(SBUF(keylet), KEYLET_SIGNERS, SBUF(carbonland_accid), 0, 0, 0, 0) != 34)
            rollback(SBUF("ESG: Internal error, could not generate keylet"), 10);

        // then requesting XRPLD slot that keylet into a new slot for us
        int64_t slot_no = slot_set(SBUF(keylet), 0);
        TRACEVAR(slot_no);
        if (slot_no < 0)
            rollback(SBUF("ESG: Could not set keylet in slot"), 10);

        // once slotted we can examine the signer list object
        // the first field we are interested in is the required quorum to actually pass a multisign transaction
        int64_t result = slot_subfield(slot_no, sfNFTokenOffers, 0);
        if (result < 0)
            rollback(SBUF("ESG: Could not find sfNFTokenOffers on hook account"), 20);

            accept(SBUF("ESG: triggered"), 2);
        }

    // accept and allow the original transaction through
    accept(SBUF("ESG: accepting"), 0);
    return 0;
}
