#!/bin/bash
#
# ***** BEGIN LICENSE BLOCK *****
#
#  BBN Address and AS Number PKI Database/repository software
#  Version 4.0
#
#  US government users are permitted unrestricted rights as
#  defined in the FAR.
#
#  This software is distributed on an "AS IS" basis, WITHOUT
#  WARRANTY OF ANY KIND, either express or implied.
#
#  Copyright (C) Raytheon BBN Technologies 2011.  All Rights Reserved.
#
#  Contributor(s): Charlie Gardiner, Andrew Chi
#
#  ***** END LICENSE BLOCK ***** */

# gen_all_CRLs.sh - create all CRL test cases for RPKI syntactic
#                   conformance test

# Set up RPKI environment variables if not already done.
THIS_SCRIPT_DIR=$(dirname $0)
. $THIS_SCRIPT_DIR/../../../envir.setup

# Safe bash shell scripting practices
. $RPKI_ROOT/trap_errors

# Usage
usage ( ) {
    usagestr="
Usage: $0 [options]

Options:
  -P        \tApply patches instead of prompting user to edit (default = false)
  -h        \tDisplay this help file

This script creates a large number of CRLs in subdirectories, prompts
the user multiple times to edit interactively (e.g., in order to
introduce errors), and captures those edits in '.patch' files (output
of diff -u).  Later, running $0 with the -P option can replay the
creation process by automatically applying those patch files instead
of prompting for user intervention.  In patch mode, existing keys are
reused from the keys directory, instead of the default of generating
new keys.

This tool assumes the repository structure in the diagram below.  It
creates a ton of subdirectories each w/ a CRL and corresponding
manifest.

NOTE: this script does NOT update the manifest issued by root.

               +-----------------------------------+
               | rsync://rpki.bbn.com/conformance/ |
               |    +--------+                     |
         +--------->|  Root  |                     |
         |     |    |  AIA   |                     |
         |     |    |  SIA   |                     |
         |     |    +---|----+                     |
         |     +--------|--------------------------+
         |              V
         |     +----------------------------------------+
         |     | rsync://rpki.bbn.com/conformance/root/ |
         |     |   +--------+     +------------+        |
         |     |   | *Child |     | CRL issued |        |
         |     |   | CRLDP------->| by Root    |        |
         +----------- AIA   |     +------------+        |
               |   |  SIA------+                        |
               |   +--------+  |  +-----------------+   |
               |               |  | Manifest issued |   |
               | Root's Repo   |  | by Root         |   |
               | Directory     |  +-----------------+   |
               +---------------|------------------------+
                               |
                               V
	     +-------------------------------------------------+
             | rsync://rpki.bbn.com/conformance/root/subjname/ |
             |                                     	       |
             |     +---------------------------+   	       |
             |     | *Manifest issued by Child |               |
             |     +---------------------------+               |
             |                                                 |
             |     +----------------------------------+        |
             |     | *CRL issued by Child (TEST CASE) |        |
             |     +----------------------------------+        |
             |                                                 |
             | *Child's Repo Directory                         |
             +-------------------------------------------------+

Inputs:
  -P - (optional) use patch mode for automatic insertion of errors

Outputs:
  child CA certificate - inherits AS/IP resources from parent via inherit bit
  child repo directory - ASSUMED to be a subdirectory of parent's repo. The
                         new directory will be <outdir>/<subjectname>/
  crl issued by child - named bad<subjectname>.crl, and has no entries
  mft issued by child - named <subjectname>.mft, and has one entry (the crl)

  The filename for the crl will be prepended by the string 'bad' by
  default, though this can be replaced by an arbitrary non-empty
  string using the -x option.

Auxiliary Outputs: (not shown in diagram)
  child key pair - <outdir>/<subjectname>.p15
  child-issued MFT EE cert - <outdir>/<subjectname>/<subjectname>.mft.cer
  child-issued MFT EE key pair - <outdir>/<subjectname>/<subjectname>.mft.p15
  patch files - manual edits are saved as diff output in
                'badCRL<subjectname>.stageN.patch' (N=0..1)
  key files - generated key pairs for child and EE certs are stored in keys dir
             as badCRL<filestem>.ee.p15
    "
    printf "${usagestr}\n"
    exit 1
}

# NOTES

# Variable naming convention -- preset constants and command line
# arguments are in ALL_CAPS.  Derived/computed values are in
# lower_case.

# Set up paths to ASN.1 tools.
CGTOOLS=$RPKI_ROOT/cg/tools	# Charlie Gardiner's tools

# Options and defaults
OUTPUT_DIR="$RPKI_ROOT/testcases/conformance/raw/root"
USE_EXISTING_PATCHES=

# Process command line arguments.
while getopts Ph opt
do
  case $opt in
      P)
	  USE_EXISTING_PATCHES=1
	  ;;
      h)
	  usage
	  ;;
  esac
done
shift $((OPTIND - 1))
if [ $# != "0" ]
then
    usage
fi

###############################################################################
# Computed Variables
###############################################################################

if [ $USE_EXISTING_PATCHES ]
then
    patch_option="-P"
else
    patch_option=
fi

single_CRL_script="$RPKI_ROOT/testcases/conformance/scripts/make_test_CRL.sh"
single_CRL_cmd="${single_CRL_script} ${patch_option} -o ${OUTPUT_DIR}"

###############################################################################
# Check for prerequisite tools and files
###############################################################################

ensure_file_exists ( ) {
    if [ ! -e "$1" ]
    then
	echo "Error: file not found - $1" 1>&2
	exit 1
    fi
}

ensure_dir_exists ( ) {
    if [ ! -d "$1" ]
    then
	echo "Error: directory not found - $1" 1>&2
	exit 1
    fi
}

ensure_dir_exists "$OUTPUT_DIR"
ensure_dir_exists "$CGTOOLS"
ensure_file_exists "${single_CRL_script}"

###############################################################################
# Generate CRL cases
###############################################################################

${single_CRL_cmd} 640 CRLNoVersion
${single_CRL_cmd} 641 CRLVersion0
${single_CRL_cmd} 642 CRLSigAlgInner
${single_CRL_cmd} 643 CRLSigAlgOuter
${single_CRL_cmd} 644 CRLIssuerOID
${single_CRL_cmd} 645 CRLIssuer2Sets
${single_CRL_cmd} 646 CRLIssuerUTF
${single_CRL_cmd} 647 CRLIssuer2Seq
${single_CRL_cmd} 648 CRLIssuer2SerNums
${single_CRL_cmd} 649 CRLThisUpdateFuture
${single_CRL_cmd} 650 CRLThisUpdateTyp
${single_CRL_cmd} 651 CRLNextUpdatePast
${single_CRL_cmd} 652 CRLNextUpdateTyp
${single_CRL_cmd} 653 CRLUpdatesCrossed
${single_CRL_cmd} 654 CRLIssAltName
${single_CRL_cmd} 655 CRLIssDistPt
${single_CRL_cmd} 656 CRLDeltaCRLInd
${single_CRL_cmd} 657 CRLNoAKI
${single_CRL_cmd} 658 CRLNoCRLNum
${single_CRL_cmd} 659 CRLEntryNoSerNum
${single_CRL_cmd} 660 CRLEntryNoRevDate
${single_CRL_cmd} 661 CRLEntryReason
