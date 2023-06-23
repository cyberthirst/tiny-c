//
// Created by Mirek Škrabal on 23.06.2023.
//

#pragma once

#define T86_REG_SZ 8
#define T86_WORD_SZ 8
//how many words are in a register - if we eg push a register to the stack
//we want to know how many words it takes up
#define REG_TO_MEM_WORD T86_REG_SZ / T86_WORD_SZ
