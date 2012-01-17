/*
 * Created on Nov 8, 2011
 */
package com.bbn.rpki.test.objects;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * Holds a list of ranges representing an allocation, free list, etc.
 *
 * @author RTomlinson
 */
public class IPRangeList implements Iterable<Range>, Constants {
  /** Indicates the address range is inherited */
  public static final IPRangeList IPV4_INHERIT = new IPRangeList(IPRangeType.ipv4);

  /** Indicates the address range is inherited */
  public static final IPRangeList IPV6_INHERIT = new IPRangeList(IPRangeType.ipv6);

  /** Indicates the address range is inherited */
  public static final IPRangeList AS_INHERIT = new IPRangeList(IPRangeType.as);

  /** Predefined constants for an empty range */
  public static final IPRangeList IPV4_EMPTY = new IPRangeList(IPRangeType.ipv4);

  /** Predefined constants for an empty range */
  public static final IPRangeList IPV6_EMPTY = new IPRangeList(IPRangeType.ipv6);

  /** Predefined constants for an empty range */
  public static final IPRangeList AS_EMPTY = new IPRangeList(IPRangeType.as);

  /**
   * @param l
   * @return true if the specified list is an inherit list
   */
  public static boolean isInherit(IPRangeList l) {
    switch(l.getIpVersion()) {
    case ipv4: return l == IPV4_INHERIT;
    case ipv6: return l == IPV6_INHERIT;
    case as: return l == AS_INHERIT;
    }
    return false;
  }

  private final IPRangeType ipVersion;
  private final List<Range> freeList;

  /**
   * @param ipVersion
   */
  public IPRangeList(IPRangeType ipVersion) {
    this.freeList = new ArrayList<Range>();
    this.ipVersion = ipVersion;
  }

  /**
   * Copy constructor
   * 
   * @param orig
   */
  public IPRangeList(IPRangeList orig) {
    freeList = new ArrayList<Range>(orig.freeList);
    this.ipVersion = orig.ipVersion;
  }

  /**
   * @param size
   * @param version
   */
  public IPRangeList(int size, IPRangeType version) {
    freeList = new ArrayList<Range>(size);
    this.ipVersion = version;
  }

  /**
   * @return the ipVersion
   */
  public IPRangeType getIpVersion() {
    return ipVersion;
  }

  /**
   * @see java.util.AbstractCollection#toString()
   */
  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder();
    for (Range range : this) {
      sb.append(",").append(range);
    }
    return sb.substring(1);
  }

  /**
   * @param min
   * @param max
   */
  public void addRange(BigInteger min, BigInteger max) {
    add(new Range(min, max, ipVersion, true));
  }

  /**
   * Allocate Internet number resources based on a free list.

    Inputs:

    free_list - [list of integer pairs] blocks of unallocated
                resources, e.g. [(3,10), (13,17), ...]

    used_list - [list of integer pairs] blocks of allocated resources,
                e.g. [(11,12), (18,34), ...]

    requests - [list of character/integer pairs] requests for resources,
                where each request block is denoted by a pair (reqtype,
                amount).  The "reqtype" field must be either 'p' (IP
                prefix) or 'r' (IP/AS range).  The "amount" field must
                be a positive integer specifying the number of
                Internet resource numbers to allocate (i.e. IP
                addresses, or AS numbers).  If "reqtype" is 'p', then
                "amount" must be a power of 2. e.g. [('r',5), ('p',
                256), ('r', 16), ...]

    range_not_prefix - [boolean] If set to True (default), returned
                ranges must NOT be expressible as a prefix.  This
                option should be set to True for IPv4/IPv6 allocation,
                and False for AS number allocation.  The default
                behavior conforms to RFC 3779 encoding requirements
                that ranges equivalent to prefixes MUST be expressed
                as prefixes.  Therefore, a "range" request must not
                return a prefix.

    Returns:

    allocated_blocks - [list of integer pairs] blocks allocated to the
                child, corresponding to the order of requests.  Note
                that the blocks MAY NOT be in ascending order of
                resource number.

    Side effects:

    The free_list and used_list will be updated to reflect the blocks
    allocated to the child.

    Exceptions:

    AllocationError - If the request cannot be fulfilled, the function
    raises an AllocationError exception.

   * @param requests
   * @param expressAsRange
   * @return the allocated ranges
   */
  public IPRangeList allocate(List<Pair> requests,
                              boolean expressAsRange) {
    IPRangeList ret = new IPRangeList(ipVersion);

    for (Pair request : requests) {
      char reqType = request.tag.charAt(0);
      BigInteger reqSize = request.arg;
      Range issuedBlock;
      if (reqType == 'r') {
        issuedBlock = allocateSingleRange(reqSize, ret, expressAsRange);
      } else if (reqType == 'p') {
        issuedBlock = allocateSinglePrefix(reqSize, ret, expressAsRange);
      } else {
        throw new AllocationError("Invalid requestType: " + reqType);
      }
      ret.freeList.add(issuedBlock);
    }
    return ret;
  }

  /**
   * @param freeList
   * @param reqSize
   * @param expressAsRange
   * @return
   */
  private Range allocateSinglePrefix(BigInteger reqSize, IPRangeList allocatedList, boolean expressAsRange) {
    if (!Range.isPowerOfTwo(reqSize)) {
      throw new AllocationError("Illegal prefix size: " + reqSize);
    }
    for (int i = 0, n = freeList.size(); i < n; i++) {
      Range x = firstFitPrefix(freeList.get(i), allocatedList, reqSize, expressAsRange);
      if (x != null) {
        removeRange(i, x);
        return x;
      }
    }
    throw new AllocationError("Unable to fulfill request for prefix of size " + reqSize);
  }

  /**
   * @param freeList
   * @param i
   * @param x
   */
  private void removeRange(int i, Range x) {
    List<Range> perforated = perforate(freeList.get(i), x);
    List<Range> insertion = freeList.subList(i, i + 1);
    insertion.clear();
    insertion.addAll(perforated);
  }

  /**
   * @param freeList
   * @param reqSize
   * @return
   */
  private Range allocateSingleRange(BigInteger reqSize,
                                    IPRangeList allocatedBlocks, boolean expressAsRange) {
    for (int i = 0, n = freeList.size(); i < n; i++) {
      Range firstRange = freeList.get(i);
      Range x = firstFitRange(firstRange, reqSize, allocatedBlocks, expressAsRange);
      if (x != null) {
        List<Range> perforated = perforate(firstRange, x);
        List<Range> insertion = freeList.subList(i,  i + 1);
        insertion.clear();
        insertion.addAll(perforated);
        return x;
      }
    }
    throw new AllocationError("Unable to fulfill request for range of size " + reqSize);
  }

  /**
   * Remove b from the enclosing range a
   * @param a
   * @param b
   * @return the resulting elements
   */
  private List<Range> perforate(Range a, Range b) {
    if (!a.contains(b)) {
      throw new AllocationError("Cannot perforate " + b + " from " + a);
    }
    List<Range> ret = new ArrayList<Range>(2);
    if (a.min.compareTo(b.min) < 0) {
      ret.add(new Range(a.min, b.min.subtract(ONE), ipVersion, true));
    }
    if (a.max.compareTo(b.max) > 0) {
      ret.add(new Range(b.max.add(ONE), a.max, ipVersion, true));
    }
    return ret;
  }

  /**
   * @param reqSize
   * @return
   */
  private Range firstFitPrefix(Range free_block, IPRangeList allocated_blocks,
                               BigInteger amount, boolean expressAsRange) {
    BigInteger search_position = free_block.min;
    while (true) {
      Range candidate = next_prefix(search_position, amount, allocated_blocks.getIpVersion(), expressAsRange);
      if (!free_block.contains(candidate)) {
        // out of resources
        break;
      }
      Range conflict = detectConflict(candidate, allocated_blocks);
      if (conflict == null) {
        return candidate;
      }
      // not overlapping or adjacent
      search_position = conflict.max.add(TWO);
    }
    return null;
  }

  private Range next_prefix(BigInteger start_pos, BigInteger amount, IPRangeType version, boolean expressAsRange) {
    // Return the next prefix of the requested size.

    if (!Range.isPowerOfTwo(amount)) {
      throw new AllocationError(String.format("Prefix request has invalid size: %d.", amount));
    }
    BigInteger next_multiple;
    if (start_pos.mod(amount).equals(BigInteger.ZERO)) {
      next_multiple = start_pos;
    } else {
      next_multiple = start_pos.divide(amount).add(BigInteger.ONE).multiply(amount);
    }
    Range prefix = new Range(next_multiple,
                             next_multiple.add(amount).subtract(BigInteger.ONE),
                             version, expressAsRange);
    if (!prefix.couldBePrefix()) {
      throw new RuntimeException(prefix + " should have been a prefix");
    }
    return prefix;
  }

  /**
   * @param reqSize
   * @return
   */
  private Range firstFitRange(Range freeRange, BigInteger reqSize, IPRangeList allocatedBlocks, boolean expressAsRange) {
    BigInteger searchPosition = freeRange.min;
    while (true) {
      Range candidate = new Range(searchPosition, searchPosition.add(reqSize).subtract(BigInteger.ONE), freeRange.version, expressAsRange);
      if (!freeRange.contains(candidate)) {
        return null;
      }
      if (expressAsRange && candidate.couldBePrefix()) {
        searchPosition = searchPosition.add(BigInteger.ONE);
        continue;
      }
      Range conflict = detectConflict(candidate, allocatedBlocks);
      if (conflict == null) {
        return candidate;
      }
      searchPosition = conflict.max.add(TWO);
    }
  }

  /**
   * @param candidate
   * @param allocatedBlocks
   * @return
   */
  private static Range detectConflict(Range candidate, IPRangeList allocatedBlocks) {
    /*Detect a resource overlap or adjacency conflict.

    Return an element (integer pair) from allocated_blocks (list of
    integer pairs) that conflicts with candidate_block (integer pair).
    An element can conflict by being numerically adjacent to the
    candidate, or by numerically overlapping with it.  If no element
    of allocated_blocks conflicts with candidate_block, return None.

    >>> detect_conflict((1,3), []) # no allocated blocks, no conflict
    >>> detect_conflict((1,3), [(7,9)]) # no conflict
    >>> detect_conflict((1,3), [(7,9), (4,5)])
    (4, 5)
     */
    Range expanded = new Range(candidate.min.subtract(BigInteger.ONE),
                               candidate.max.add(BigInteger.ONE),
                               candidate.version, true);
    for (Range a : allocatedBlocks) {
      if (a.overlaps(expanded)) {
        return a;
      }
    }
    return null;
  }

  /**
   * @param range
   */
  public void add(Range range) {
    for (int i = 0, n = freeList.size(); i < n; i++) {
      Range test = freeList.get(i);
      Range test2 = null;
      if (test.compareTo(range) > 0) {
        assert !test.overlaps(range);
        // There are four legal cases:
        //  range is adjacent to test
        //  range is adjacent to the preceding range
        //  range is adjacent to neither
        //  range is adjacent to both
        int x = 0;
        if (range.max.equals(test.min.subtract(BigInteger.ONE))) {
          // Adjacent to test
          x |= 1;
        }
        if (i > 0) {
          // May be adjacent to the preceding range
          test2 = freeList.get(i - 1);
          assert !test2.overlaps(range);
          if (range.min.equals(test2.max.add(BigInteger.ONE))) {
            x |= 2;
          }
        }
        switch (x) {
        case 0:
          // Not adjacent at all
          freeList.add(i, range);
          return;
        case 1:
          // Adjacent to following range
          test.min = range.min;
          return;
        case 2:
          // Adjacent to preceding range
          test2.max = range.max;
          return;
        case 3:
          // adjacent to both
          test2.max = test.max;
          freeList.remove(i);
          return;
        }
        return;
      }
    }
    freeList.add(range);
  }
  /**
   * @param range
   */
  public void remove(Range range) {
    for (int i = 0, n = freeList.size(); i < n; i++) {
      Range test = freeList.get(i);
      if (test.min.compareTo(range.max) > 0) {
        return;
      }
      Range intersection = test.intersection(range);
      if (intersection != null) {
        freeList.set(i, intersection);
      }
    }
  }

  /**
   * @param allocation
   */
  public void addAll(IPRangeList allocation) {
    for (Range range : allocation) {
      add(range);
    }
  }

  /**
   * @param allocationIndex
   * @return the Range at the specified index
   */
  public Range get(int allocationIndex) {
    return freeList.get(allocationIndex);
  }

  /**
   * @see java.lang.Iterable#iterator()
   */
  @Override
  public Iterator<Range> iterator() {
    return Collections.unmodifiableList(freeList).iterator();
  }
}