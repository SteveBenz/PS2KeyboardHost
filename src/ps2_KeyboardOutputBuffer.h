#pragma once

#include "ps2_KeyboardOutput.h"
#include "ps2_NullDiagnostics.h"
#include <util/atomic.h>

namespace ps2 {
	template <uint8_t Size, typename TDiagnostics = NullDiagnostics>
	class KeyboardOutputBuffer {
		volatile uint8_t head;
		volatile uint8_t tail;
		volatile KeyboardOutput buffer[Size];
		TDiagnostics *diagnostics;

		const uint8_t EmptyMarker = 0xff;

	public:
		KeyboardOutputBuffer(TDiagnostics &diagnostics) {
			this->head = EmptyMarker;
            this->tail = 0;
            this->diagnostics = &diagnostics;
		};

		/** Enqueues the given data from the keyboard.  This code
		 *  expects to be run from inside an interrupt handler
		 */
		void push(KeyboardOutput valueAtTop) {
			uint8_t nextTail = (tail + 1) % Size;
			if (head == EmptyMarker) {
				head = tail;
			}
			else if (head == tail) {
				this->diagnostics->bufferOverflow();
				++head;
			}
			buffer[tail] = valueAtTop;
			tail = nextTail;
		}

		/** If the queue has any content, it returns the value.  if there
		 *  is no data, it returns KeyboardOutput::none
		 */
		KeyboardOutput pop() {
			KeyboardOutput valueAtTop;
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				if (head == EmptyMarker) {
					valueAtTop = KeyboardOutput::none;
				}
				else {
					valueAtTop = buffer[head];
					head = (head + 1) % Size;
					if (head == tail) {
						head = EmptyMarker;
					}
				}
			}
			return valueAtTop;
		}

		void clear() {
			ATOMIC_BLOCK(ATOMIC_FORCEON) {
				head = EmptyMarker;
			}
		}
	};


    // If the only thing you're driving is the keyboard, then you really don't need a multi-byte buffer.

    template <typename TDiagnostics>
    class KeyboardOutputBuffer<1, TDiagnostics> {
        volatile KeyboardOutput buffer;
        TDiagnostics *diagnostics;

    public:
        KeyboardOutputBuffer(TDiagnostics &diagnostics) {
            this->diagnostics = &diagnostics;
            this->buffer = KeyboardOutput::none;
        };

        /** Enqueues the given data from the keyboard.  This code
        *  expects to be run from inside an interrupt handler
        */
        void push(KeyboardOutput valueAtTop) {
            if (buffer != KeyboardOutput::none) {
                this->diagnostics->bufferOverflow();
            }
            this->buffer = valueAtTop;
        }

        /** If the queue has any content, it returns the value.  if there
        *  is no data, it returns KeyboardOutput::none
        */
        KeyboardOutput pop() {
            KeyboardOutput valueAtTop;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                valueAtTop = buffer;
                buffer = KeyboardOutput::none;
            }
            return valueAtTop;
        }

        void clear() {
            buffer = KeyboardOutput::none;
        }
    };
}