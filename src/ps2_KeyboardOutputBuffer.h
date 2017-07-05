/*
Copyright (C) 2017 Steve Benz <s8878992@hotmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/
#pragma once

#include "ps2_KeyboardOutput.h"
#include "ps2_NullDiagnostics.h"
#include <util/atomic.h>

namespace ps2 {
    /** @private
     *  A class for buffering the data coming from the PS2.
     */
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

    /** @private
    *  A class for buffering the data coming from the PS2, specialized for one byte.
    */
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