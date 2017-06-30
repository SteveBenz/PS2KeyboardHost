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
		KeyboardOutputBuffer(const TDiagnostics diagnostics) {
			head = EmptyMarker;
			tail = 0;
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
				this->diagnostics->BufferOverflow();
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


		// Functors that allow this to be composed with translators
		KeyboardOutput operator()() { KeyboardOutput o; this->pop(o); return o; }
		void operator()(KeyboardOutput o) { this->push(o); }
	};

#if 0
	template <uint8_t Size, typename TDiagnostics> class KeyboardOutputBuffer<1, TDiagnostics>
	class KeyboardOutputBuffer {
		volatile KeyboardOutput buffer;
		TDiagnostics *diagnostics;

	public:
		KeyboardOutputBuffer(const TDiagnostics diagnostics) {
			buffer = KeyboardOutput::none;
		};

		/** Enqueues the given data from the keyboard.  This code
		*  expects to be run from inside an interrupt handler
		*/
		void push(KeyboardOutput valueAtTop) {
			if (buffer != KeyboardOutput::none) {
				this->diagnostics->BufferOverflow();
			}
			buffer = valueAtTop;
		}

		/** If the queue has any content, it returns true and sets
		* valueAtTop to the least-recently-pushed byte of output.
		* If it does not contain anything, then it returns false and
		* sets valueAtTop to KeyboardOutput::none.
		*/
		bool pop(KeyboardOutput &valueAtTop) {
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				valueAtTop = buffer;
				buffer = KeyboardOutput::none;
			}
			return valueAtTop != KeyboardOutput::none;
		}

		void clear() {
			buffer = KeyboardOutput::none;
		}

		// Functors that allow this to be composed with translators
		KeyboardOutput operator()() { KeyboardOutput o; this->pop(o); return o; }
		void operator()(KeyboardOutput o) { this->push(o); }
	};
#endif
}