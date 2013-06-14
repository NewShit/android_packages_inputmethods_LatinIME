/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LATINIME_BINARY_FORMAT_H
#define LATINIME_BINARY_FORMAT_H

#include <stdint.h>

#include "suggest/core/dictionary/probability_utils.h"
#include "utils/char_utils.h"

namespace latinime {

class BinaryFormat {
 public:
    // Mask and flags for children address type selection.
    static const int MASK_GROUP_ADDRESS_TYPE = 0xC0;

    // Flag for single/multiple char group
    static const int FLAG_HAS_MULTIPLE_CHARS = 0x20;

    // Flag for terminal groups
    static const int FLAG_IS_TERMINAL = 0x10;

    // Flag for shortcut targets presence
    static const int FLAG_HAS_SHORTCUT_TARGETS = 0x08;
    // Flag for bigram presence
    static const int FLAG_HAS_BIGRAMS = 0x04;
    // Flag for non-words (typically, shortcut only entries)
    static const int FLAG_IS_NOT_A_WORD = 0x02;
    // Flag for blacklist
    static const int FLAG_IS_BLACKLISTED = 0x01;

    // Attribute (bigram/shortcut) related flags:
    // Flag for presence of more attributes
    static const int FLAG_ATTRIBUTE_HAS_NEXT = 0x80;
    // Flag for sign of offset. If this flag is set, the offset value must be negated.
    static const int FLAG_ATTRIBUTE_OFFSET_NEGATIVE = 0x40;

    // Mask for attribute probability, stored on 4 bits inside the flags byte.
    static const int MASK_ATTRIBUTE_PROBABILITY = 0x0F;
    // The numeric value of the shortcut probability that means 'whitelist'.
    static const int WHITELIST_SHORTCUT_PROBABILITY = 15;

    // Mask and flags for attribute address type selection.
    static const int MASK_ATTRIBUTE_ADDRESS_TYPE = 0x30;

    static const int SHORTCUT_LIST_SIZE_SIZE = 2;

    static bool hasBlacklistedOrNotAWordFlag(const int flags);
    static int getGroupCountAndForwardPointer(const uint8_t *const dict, int *pos);
    static uint8_t getFlagsAndForwardPointer(const uint8_t *const dict, int *pos);
    static int getCodePointAndForwardPointer(const uint8_t *const dict, int *pos);
    static int readProbabilityWithoutMovingPointer(const uint8_t *const dict, const int pos);
    static int skipOtherCharacters(const uint8_t *const dict, const int pos);
    static int skipChildrenPosition(const uint8_t flags, const int pos);
    static int skipProbability(const uint8_t flags, const int pos);
    static int skipShortcuts(const uint8_t *const dict, const uint8_t flags, const int pos);
    static int skipChildrenPosAndAttributes(const uint8_t *const dict, const uint8_t flags,
            const int pos);
    static int readChildrenPosition(const uint8_t *const dict, const uint8_t flags, const int pos);
    static bool hasChildrenInFlags(const uint8_t flags);
    static int getAttributeAddressAndForwardPointer(const uint8_t *const dict, const uint8_t flags,
            int *pos);
    static int getAttributeProbabilityFromFlags(const int flags);
    static int getTerminalPosition(const uint8_t *const root, const int *const inWord,
            const int length, const bool forceLowerCaseSearch);
    static int getWordAtAddress(const uint8_t *const root, const int address, const int maxDepth,
            int *outWord, int *outUnigramProbability);
    static int getBigramListPositionForWordPosition(const uint8_t *const root, int position);

 private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(BinaryFormat);

    static const int FLAG_GROUP_ADDRESS_TYPE_NOADDRESS = 0x00;
    static const int FLAG_GROUP_ADDRESS_TYPE_ONEBYTE = 0x40;
    static const int FLAG_GROUP_ADDRESS_TYPE_TWOBYTES = 0x80;
    static const int FLAG_GROUP_ADDRESS_TYPE_THREEBYTES = 0xC0;
    static const int FLAG_ATTRIBUTE_ADDRESS_TYPE_ONEBYTE = 0x10;
    static const int FLAG_ATTRIBUTE_ADDRESS_TYPE_TWOBYTES = 0x20;
    static const int FLAG_ATTRIBUTE_ADDRESS_TYPE_THREEBYTES = 0x30;

    static const int CHARACTER_ARRAY_TERMINATOR_SIZE = 1;
    static const int MINIMAL_ONE_BYTE_CHARACTER_VALUE = 0x20;
    static const int CHARACTER_ARRAY_TERMINATOR = 0x1F;
    static const int MULTIPLE_BYTE_CHARACTER_ADDITIONAL_SIZE = 2;
    static const int NO_FLAGS = 0;
    static int skipAllAttributes(const uint8_t *const dict, const uint8_t flags, const int pos);
    static int skipBigrams(const uint8_t *const dict, const uint8_t flags, const int pos);
};

inline bool BinaryFormat::hasBlacklistedOrNotAWordFlag(const int flags) {
    return (flags & (FLAG_IS_BLACKLISTED | FLAG_IS_NOT_A_WORD)) != 0;
}

AK_FORCE_INLINE int BinaryFormat::getGroupCountAndForwardPointer(const uint8_t *const dict,
        int *pos) {
    const int msb = dict[(*pos)++];
    if (msb < 0x80) return msb;
    return ((msb & 0x7F) << 8) | dict[(*pos)++];
}

inline uint8_t BinaryFormat::getFlagsAndForwardPointer(const uint8_t *const dict, int *pos) {
    return dict[(*pos)++];
}

AK_FORCE_INLINE int BinaryFormat::getCodePointAndForwardPointer(const uint8_t *const dict,
        int *pos) {
    const int origin = *pos;
    const int codePoint = dict[origin];
    if (codePoint < MINIMAL_ONE_BYTE_CHARACTER_VALUE) {
        if (codePoint == CHARACTER_ARRAY_TERMINATOR) {
            *pos = origin + 1;
            return NOT_A_CODE_POINT;
        } else {
            *pos = origin + 3;
            const int char_1 = codePoint << 16;
            const int char_2 = char_1 + (dict[origin + 1] << 8);
            return char_2 + dict[origin + 2];
        }
    } else {
        *pos = origin + 1;
        return codePoint;
    }
}

inline int BinaryFormat::readProbabilityWithoutMovingPointer(const uint8_t *const dict,
        const int pos) {
    return dict[pos];
}

AK_FORCE_INLINE int BinaryFormat::skipOtherCharacters(const uint8_t *const dict, const int pos) {
    int currentPos = pos;
    int character = dict[currentPos++];
    while (CHARACTER_ARRAY_TERMINATOR != character) {
        if (character < MINIMAL_ONE_BYTE_CHARACTER_VALUE) {
            currentPos += MULTIPLE_BYTE_CHARACTER_ADDITIONAL_SIZE;
        }
        character = dict[currentPos++];
    }
    return currentPos;
}

static inline int attributeAddressSize(const uint8_t flags) {
    static const int ATTRIBUTE_ADDRESS_SHIFT = 4;
    return (flags & BinaryFormat::MASK_ATTRIBUTE_ADDRESS_TYPE) >> ATTRIBUTE_ADDRESS_SHIFT;
    /* Note: this is a value-dependant optimization of what may probably be
       more readably written this way:
       switch (flags * BinaryFormat::MASK_ATTRIBUTE_ADDRESS_TYPE) {
       case FLAG_ATTRIBUTE_ADDRESS_TYPE_ONEBYTE: return 1;
       case FLAG_ATTRIBUTE_ADDRESS_TYPE_TWOBYTES: return 2;
       case FLAG_ATTRIBUTE_ADDRESS_TYPE_THREEBYTE: return 3;
       default: return 0;
       }
    */
}

static AK_FORCE_INLINE int skipExistingBigrams(const uint8_t *const dict, const int pos) {
    int currentPos = pos;
    uint8_t flags = BinaryFormat::getFlagsAndForwardPointer(dict, &currentPos);
    while (flags & BinaryFormat::FLAG_ATTRIBUTE_HAS_NEXT) {
        currentPos += attributeAddressSize(flags);
        flags = BinaryFormat::getFlagsAndForwardPointer(dict, &currentPos);
    }
    currentPos += attributeAddressSize(flags);
    return currentPos;
}

static inline int childrenAddressSize(const uint8_t flags) {
    static const int CHILDREN_ADDRESS_SHIFT = 6;
    return (BinaryFormat::MASK_GROUP_ADDRESS_TYPE & flags) >> CHILDREN_ADDRESS_SHIFT;
    /* See the note in attributeAddressSize. The same applies here */
}

static AK_FORCE_INLINE int shortcutByteSize(const uint8_t *const dict, const int pos) {
    return (static_cast<int>(dict[pos] << 8)) + (dict[pos + 1]);
}

inline int BinaryFormat::skipChildrenPosition(const uint8_t flags, const int pos) {
    return pos + childrenAddressSize(flags);
}

inline int BinaryFormat::skipProbability(const uint8_t flags, const int pos) {
    return FLAG_IS_TERMINAL & flags ? pos + 1 : pos;
}

AK_FORCE_INLINE int BinaryFormat::skipShortcuts(const uint8_t *const dict, const uint8_t flags,
        const int pos) {
    if (FLAG_HAS_SHORTCUT_TARGETS & flags) {
        return pos + shortcutByteSize(dict, pos);
    } else {
        return pos;
    }
}

AK_FORCE_INLINE int BinaryFormat::skipBigrams(const uint8_t *const dict, const uint8_t flags,
        const int pos) {
    if (FLAG_HAS_BIGRAMS & flags) {
        return skipExistingBigrams(dict, pos);
    } else {
        return pos;
    }
}

AK_FORCE_INLINE int BinaryFormat::skipAllAttributes(const uint8_t *const dict, const uint8_t flags,
        const int pos) {
    // This function skips all attributes: shortcuts and bigrams.
    int newPos = pos;
    newPos = skipShortcuts(dict, flags, newPos);
    newPos = skipBigrams(dict, flags, newPos);
    return newPos;
}

AK_FORCE_INLINE int BinaryFormat::skipChildrenPosAndAttributes(const uint8_t *const dict,
        const uint8_t flags, const int pos) {
    int currentPos = pos;
    currentPos = skipChildrenPosition(flags, currentPos);
    currentPos = skipAllAttributes(dict, flags, currentPos);
    return currentPos;
}

AK_FORCE_INLINE int BinaryFormat::readChildrenPosition(const uint8_t *const dict,
        const uint8_t flags, const int pos) {
    int offset = 0;
    switch (MASK_GROUP_ADDRESS_TYPE & flags) {
        case FLAG_GROUP_ADDRESS_TYPE_ONEBYTE:
            offset = dict[pos];
            break;
        case FLAG_GROUP_ADDRESS_TYPE_TWOBYTES:
            offset = dict[pos] << 8;
            offset += dict[pos + 1];
            break;
        case FLAG_GROUP_ADDRESS_TYPE_THREEBYTES:
            offset = dict[pos] << 16;
            offset += dict[pos + 1] << 8;
            offset += dict[pos + 2];
            break;
        default:
            // If we come here, it means we asked for the children of a word with
            // no children.
            return -1;
    }
    return pos + offset;
}

inline bool BinaryFormat::hasChildrenInFlags(const uint8_t flags) {
    return (FLAG_GROUP_ADDRESS_TYPE_NOADDRESS != (MASK_GROUP_ADDRESS_TYPE & flags));
}

AK_FORCE_INLINE int BinaryFormat::getAttributeAddressAndForwardPointer(const uint8_t *const dict,
        const uint8_t flags, int *pos) {
    int offset = 0;
    const int origin = *pos;
    switch (MASK_ATTRIBUTE_ADDRESS_TYPE & flags) {
        case FLAG_ATTRIBUTE_ADDRESS_TYPE_ONEBYTE:
            offset = dict[origin];
            *pos = origin + 1;
            break;
        case FLAG_ATTRIBUTE_ADDRESS_TYPE_TWOBYTES:
            offset = dict[origin] << 8;
            offset += dict[origin + 1];
            *pos = origin + 2;
            break;
        case FLAG_ATTRIBUTE_ADDRESS_TYPE_THREEBYTES:
            offset = dict[origin] << 16;
            offset += dict[origin + 1] << 8;
            offset += dict[origin + 2];
            *pos = origin + 3;
            break;
    }
    if (FLAG_ATTRIBUTE_OFFSET_NEGATIVE & flags) {
        return origin - offset;
    } else {
        return origin + offset;
    }
}

inline int BinaryFormat::getAttributeProbabilityFromFlags(const int flags) {
    return flags & MASK_ATTRIBUTE_PROBABILITY;
}

// This function gets the byte position of the last chargroup of the exact matching word in the
// dictionary. If no match is found, it returns NOT_VALID_WORD.
AK_FORCE_INLINE int BinaryFormat::getTerminalPosition(const uint8_t *const root,
        const int *const inWord, const int length, const bool forceLowerCaseSearch) {
    int pos = 0;
    int wordPos = 0;

    while (true) {
        // If we already traversed the tree further than the word is long, there means
        // there was no match (or we would have found it).
        if (wordPos >= length) return NOT_VALID_WORD;
        int charGroupCount = BinaryFormat::getGroupCountAndForwardPointer(root, &pos);
        const int wChar = forceLowerCaseSearch
                ? CharUtils::toLowerCase(inWord[wordPos]) : inWord[wordPos];
        while (true) {
            // If there are no more character groups in this node, it means we could not
            // find a matching character for this depth, therefore there is no match.
            if (0 >= charGroupCount) return NOT_VALID_WORD;
            const int charGroupPos = pos;
            const uint8_t flags = BinaryFormat::getFlagsAndForwardPointer(root, &pos);
            int character = BinaryFormat::getCodePointAndForwardPointer(root, &pos);
            if (character == wChar) {
                // This is the correct node. Only one character group may start with the same
                // char within a node, so either we found our match in this node, or there is
                // no match and we can return NOT_VALID_WORD. So we will check all the characters
                // in this character group indeed does match.
                if (FLAG_HAS_MULTIPLE_CHARS & flags) {
                    character = BinaryFormat::getCodePointAndForwardPointer(root, &pos);
                    while (NOT_A_CODE_POINT != character) {
                        ++wordPos;
                        // If we shoot the length of the word we search for, or if we find a single
                        // character that does not match, as explained above, it means the word is
                        // not in the dictionary (by virtue of this chargroup being the only one to
                        // match the word on the first character, but not matching the whole word).
                        if (wordPos >= length) return NOT_VALID_WORD;
                        if (inWord[wordPos] != character) return NOT_VALID_WORD;
                        character = BinaryFormat::getCodePointAndForwardPointer(root, &pos);
                    }
                }
                // If we come here we know that so far, we do match. Either we are on a terminal
                // and we match the length, in which case we found it, or we traverse children.
                // If we don't match the length AND don't have children, then a word in the
                // dictionary fully matches a prefix of the searched word but not the full word.
                ++wordPos;
                if (FLAG_IS_TERMINAL & flags) {
                    if (wordPos == length) {
                        return charGroupPos;
                    }
                    pos = BinaryFormat::skipProbability(FLAG_IS_TERMINAL, pos);
                }
                if (FLAG_GROUP_ADDRESS_TYPE_NOADDRESS == (MASK_GROUP_ADDRESS_TYPE & flags)) {
                    return NOT_VALID_WORD;
                }
                // We have children and we are still shorter than the word we are searching for, so
                // we need to traverse children. Put the pointer on the children position, and
                // break
                pos = BinaryFormat::readChildrenPosition(root, flags, pos);
                break;
            } else {
                // This chargroup does not match, so skip the remaining part and go to the next.
                if (FLAG_HAS_MULTIPLE_CHARS & flags) {
                    pos = BinaryFormat::skipOtherCharacters(root, pos);
                }
                pos = BinaryFormat::skipProbability(flags, pos);
                pos = BinaryFormat::skipChildrenPosAndAttributes(root, flags, pos);
            }
            --charGroupCount;
        }
    }
}

// This function searches for a terminal in the dictionary by its address.
// Due to the fact that words are ordered in the dictionary in a strict breadth-first order,
// it is possible to check for this with advantageous complexity. For each node, we search
// for groups with children and compare the children address with the address we look for.
// When we shoot the address we look for, it means the word we look for is in the children
// of the previous group. The only tricky part is the fact that if we arrive at the end of a
// node with the last group's children address still less than what we are searching for, we
// must descend the last group's children (for example, if the word we are searching for starts
// with a z, it's the last group of the root node, so all children addresses will be smaller
// than the address we look for, and we have to descend the z node).
/* Parameters :
 * root: the dictionary buffer
 * address: the byte position of the last chargroup of the word we are searching for (this is
 *   what is stored as the "bigram address" in each bigram)
 * outword: an array to write the found word, with MAX_WORD_LENGTH size.
 * outUnigramProbability: a pointer to an int to write the probability into.
 * Return value : the length of the word, of 0 if the word was not found.
 */
AK_FORCE_INLINE int BinaryFormat::getWordAtAddress(const uint8_t *const root, const int address,
        const int maxDepth, int *outWord, int *outUnigramProbability) {
    int pos = 0;
    int wordPos = 0;

    // One iteration of the outer loop iterates through nodes. As stated above, we will only
    // traverse nodes that are actually a part of the terminal we are searching, so each time
    // we enter this loop we are one depth level further than last time.
    // The only reason we count nodes is because we want to reduce the probability of infinite
    // looping in case there is a bug. Since we know there is an upper bound to the depth we are
    // supposed to traverse, it does not hurt to count iterations.
    for (int loopCount = maxDepth; loopCount > 0; --loopCount) {
        int lastCandidateGroupPos = 0;
        // Let's loop through char groups in this node searching for either the terminal
        // or one of its ascendants.
        for (int charGroupCount = getGroupCountAndForwardPointer(root, &pos); charGroupCount > 0;
                 --charGroupCount) {
            const int startPos = pos;
            const uint8_t flags = getFlagsAndForwardPointer(root, &pos);
            const int character = getCodePointAndForwardPointer(root, &pos);
            if (address == startPos) {
                // We found the address. Copy the rest of the word in the buffer and return
                // the length.
                outWord[wordPos] = character;
                if (FLAG_HAS_MULTIPLE_CHARS & flags) {
                    int nextChar = getCodePointAndForwardPointer(root, &pos);
                    // We count chars in order to avoid infinite loops if the file is broken or
                    // if there is some other bug
                    int charCount = maxDepth;
                    while (NOT_A_CODE_POINT != nextChar && --charCount > 0) {
                        outWord[++wordPos] = nextChar;
                        nextChar = getCodePointAndForwardPointer(root, &pos);
                    }
                }
                *outUnigramProbability = readProbabilityWithoutMovingPointer(root, pos);
                return ++wordPos;
            }
            // We need to skip past this char group, so skip any remaining chars after the
            // first and possibly the probability.
            if (FLAG_HAS_MULTIPLE_CHARS & flags) {
                pos = skipOtherCharacters(root, pos);
            }
            pos = skipProbability(flags, pos);

            // The fact that this group has children is very important. Since we already know
            // that this group does not match, if it has no children we know it is irrelevant
            // to what we are searching for.
            const bool hasChildren = (FLAG_GROUP_ADDRESS_TYPE_NOADDRESS !=
                    (MASK_GROUP_ADDRESS_TYPE & flags));
            // We will write in `found' whether we have passed the children address we are
            // searching for. For example if we search for "beer", the children of b are less
            // than the address we are searching for and the children of c are greater. When we
            // come here for c, we realize this is too big, and that we should descend b.
            bool found;
            if (hasChildren) {
                // Here comes the tricky part. First, read the children position.
                const int childrenPos = readChildrenPosition(root, flags, pos);
                if (childrenPos > address) {
                    // If the children pos is greater than address, it means the previous chargroup,
                    // which address is stored in lastCandidateGroupPos, was the right one.
                    found = true;
                } else if (1 >= charGroupCount) {
                    // However if we are on the LAST group of this node, and we have NOT shot the
                    // address we should descend THIS node. So we trick the lastCandidateGroupPos
                    // so that we will descend this node, not the previous one.
                    lastCandidateGroupPos = startPos;
                    found = true;
                } else {
                    // Else, we should continue looking.
                    found = false;
                }
            } else {
                // Even if we don't have children here, we could still be on the last group of this
                // node. If this is the case, we should descend the last group that had children,
                // and their address is already in lastCandidateGroup.
                found = (1 >= charGroupCount);
            }

            if (found) {
                // Okay, we found the group we should descend. Its address is in
                // the lastCandidateGroupPos variable, so we just re-read it.
                if (0 != lastCandidateGroupPos) {
                    const uint8_t lastFlags =
                            getFlagsAndForwardPointer(root, &lastCandidateGroupPos);
                    const int lastChar =
                            getCodePointAndForwardPointer(root, &lastCandidateGroupPos);
                    // We copy all the characters in this group to the buffer
                    outWord[wordPos] = lastChar;
                    if (FLAG_HAS_MULTIPLE_CHARS & lastFlags) {
                        int nextChar = getCodePointAndForwardPointer(root, &lastCandidateGroupPos);
                        int charCount = maxDepth;
                        while (-1 != nextChar && --charCount > 0) {
                            outWord[++wordPos] = nextChar;
                            nextChar = getCodePointAndForwardPointer(root, &lastCandidateGroupPos);
                        }
                    }
                    ++wordPos;
                    // Now we only need to branch to the children address. Skip the probability if
                    // it's there, read pos, and break to resume the search at pos.
                    lastCandidateGroupPos = skipProbability(lastFlags, lastCandidateGroupPos);
                    pos = readChildrenPosition(root, lastFlags, lastCandidateGroupPos);
                    break;
                } else {
                    // Here is a little tricky part: we come here if we found out that all children
                    // addresses in this group are bigger than the address we are searching for.
                    // Should we conclude the word is not in the dictionary? No! It could still be
                    // one of the remaining chargroups in this node, so we have to keep looking in
                    // this node until we find it (or we realize it's not there either, in which
                    // case it's actually not in the dictionary). Pass the end of this group, ready
                    // to start the next one.
                    pos = skipChildrenPosAndAttributes(root, flags, pos);
                }
            } else {
                // If we did not find it, we should record the last children address for the next
                // iteration.
                if (hasChildren) lastCandidateGroupPos = startPos;
                // Now skip the end of this group (children pos and the attributes if any) so that
                // our pos is after the end of this char group, at the start of the next one.
                pos = skipChildrenPosAndAttributes(root, flags, pos);
            }

        }
    }
    // If we have looked through all the chargroups and found no match, the address is
    // not the address of a terminal in this dictionary.
    return 0;
}

AK_FORCE_INLINE int BinaryFormat::getBigramListPositionForWordPosition(
        const uint8_t *const root, int position) {
    if (NOT_VALID_WORD == position) return 0;
    const uint8_t flags = getFlagsAndForwardPointer(root, &position);
    if (!(flags & FLAG_HAS_BIGRAMS)) return 0;
    if (flags & FLAG_HAS_MULTIPLE_CHARS) {
        position = skipOtherCharacters(root, position);
    } else {
        getCodePointAndForwardPointer(root, &position);
    }
    position = skipProbability(flags, position);
    position = skipChildrenPosition(flags, position);
    position = skipShortcuts(root, flags, position);
    return position;
}

} // namespace latinime
#endif // LATINIME_BINARY_FORMAT_H
