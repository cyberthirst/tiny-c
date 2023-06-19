//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

class RegAllocator {

};

/*
 * Allocates abstract registers, ie ignores the limit set by the architecture.
 * Used during the instruction selection phase, the resulting target
 * code has to be further processed by a real register allocator.
 */
class AbstractRegAllocator : public RegAllocator {

};