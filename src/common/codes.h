
// those are just constant that will be used conveniabily by the files that will include this one




OPCODE(CONSTANT , 1)            // = 0
OPCODE(NULL , 1)                // = 1
OPCODE(FALSE , 1)               // = 2
OPCODE(TRUE , 1)                // = 3
OPCODE(VOID , 1)                // = 4

OPCODE(LOAD_LOCAL_0 , 1)        // = 5
OPCODE(LOAD_LOCAL_1 , 1)        // = 6
OPCODE(LOAD_LOCAL_2 , 1)        // = 7
OPCODE(LOAD_LOCAL_3 , 1)        // = 8
OPCODE(LOAD_LOCAL_4 , 1)        // = 9
OPCODE(LOAD_LOCAL_5 , 1)        // = 10
OPCODE(LOAD_LOCAL_6 , 1)        // = 11
OPCODE(LOAD_LOCAL_7 , 1)        // = 12
OPCODE(LOAD_LOCAL_8 , 1)        // = 13

OPCODE(LOAD_LOCAL , 1)          // = 14

OPCODE(STORE_LOCAL , 0)         // = 15

// Pushes the value in upvalue [arg].
OPCODE(LOAD_UPVALUE, 1)         // = 16

// Stores the top of stack in upvalue [arg]. Does not pop it.
OPCODE(STORE_UPVALUE, 0)        // = 17

OPCODE(LOAD_MODULE_VAR , 1)     // = 18
OPCODE(STORE_MODULE_VAR , 0)    // = 19


OPCODE(LOAD_FIELD_THIS , 1)     // = 20
OPCODE(STORE_FIELD_THIS , 0)    // = 21

OPCODE(LOAD_FIELD , 1)          // = 22
OPCODE(STORE_FIELD , -1)        // = 23

OPCODE(POP , -1)                // = 24

OPCODE(CALL_0 , 0)              // = 25
OPCODE(CALL_1 , -1)             // = 26
OPCODE(CALL_2 , -2)             // = 27
OPCODE(CALL_3 , -3)             // = 28
OPCODE(CALL_4 , -4)             // = 29
OPCODE(CALL_5 , -5)             // = 30
OPCODE(CALL_6 , -6)             // = 31
OPCODE(CALL_7 , -7)             // = 32
OPCODE(CALL_8 , -8)             // = 33
OPCODE(CALL_9 , -9)             // = 34
OPCODE(CALL_10 , -10)           // = 35
OPCODE(CALL_11 , -11)           // = 36
OPCODE(CALL_12 , -12)           // = 37
OPCODE(CALL_13 , -13)           // = 38
OPCODE(CALL_14 , -14)           // = 39
OPCODE(CALL_15 , -15)           // = 40
OPCODE(CALL_16 , -16)           // = 41

OPCODE(SUPER_0 , 0)             // = 42
OPCODE(SUPER_1 , -1)            // = 43
OPCODE(SUPER_2 , -2)            // = 44
OPCODE(SUPER_3 , -3)            // = 45
OPCODE(SUPER_4 , -4)            // = 46
OPCODE(SUPER_5 , -5)            // = 47
OPCODE(SUPER_6 , -6)            // = 48
OPCODE(SUPER_7 , -7)            // = 49
OPCODE(SUPER_8 , -8)            // = 50
OPCODE(SUPER_9 , -9)            // = 51
OPCODE(SUPER_10 , -10)          // = 52
OPCODE(SUPER_11 , -11)          // = 53
OPCODE(SUPER_12 , -12)          // = 54
OPCODE(SUPER_13 , -13)          // = 55
OPCODE(SUPER_14 ,-14)           // = 56
OPCODE(SUPER_15 , -15)          // = 57
OPCODE(SUPER_16 , -16)          // = 58

OPCODE(JUMP , 0)                // = 59
OPCODE(LOOP , 0)                // = 60
OPCODE(JUMP_IF , -1)            // = 61
OPCODE(AND , -1)                // = 62
OPCODE(OR , -1)                 // = 63

OPCODE(CLOSE_UPVALUE, -1)       // = 64

OPCODE(RETURN, 0)               // = 65

OPCODE(CLOSURE , 1)             // = 66


OPCODE(CONSTRUCT , 0)           // = 67

OPCODE(EXTERN_CONSTRUCT , 0)    // = 68

OPCODE(CLASS , -1)              // = 69

OPCODE(END_CLASS , -2)          // = 70

OPCODE(EXTERN_CLASS , -1)       // = 71

OPCODE(METHOD_INSTANCE , -2)    // = 72

OPCODE(METHOD_STATIC , -2)      // = 73

OPCODE(END_MODULE , 1)          // = 74

OPCODE(IMPORT_MODULE , 1)       // = 75


OPCODE(IMPORT_VARIABLE , 1)     // = 76

OPCODE(CALL, 0)                 // = 77

OPCODE(FIELD, 0)                 // = 78

OPCODE(END , 0)                 // = 79
