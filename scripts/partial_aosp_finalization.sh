#!/bin/bash
# partially freeze AOSP HALs
#
#
if [ $# != 2 ]; then
  echo "TODO bad!"
  exit 1
fi

pushd .
AOSP_REPO=~/workspace/aosp/main
INTERNAL_REPO=~/workspace/internal/main
INTERFACES=hardware/interfaces
SYSTEM_INTERFACES=system/hardware/interfaces
FRAMEWORKS_INTERFACES=frameworks/hardware/interfaces

# AIDL_TRANSITIVE_FREEZE=true m aidl-freeze-api

function freeze_project() {
  cd "$AOSP_REPO/$1"
  if [ -z "$(git status -s)" ]; then
    echo ""
#    echo "$1 no changes!"
# TODO since I already commited the freeze
#    return
  fi

  MODIFIED_HASHES=$(git diff HEAD~1 --name-only | grep hash)
  AOSP_HASHES=
  INTERNAL_HASHES=
  for h in $MODIFIED_HASHES
  do
    AOSP_HASHES+="$h $(cat $AOSP_REPO/$1/$h)\n"
    INTERNAL_HASHES+="$h $(cat $INTERNAL_REPO/$1/$h)\n"
  done

  diff <(echo -e "${AOSP_HASHES}") <(echo -e "$INTERNAL_HASHES")

  # TODO now take that diff and extract the top level directory for the
  # interface.
  #
  # rm -rf $dir
  # git checkout HEAD~1 $dir
  # git add *
  # git commit
  # Need to add the Merged-In ID of the internal CL - so add that as another arg
  # to the script

  echo "Done finalizing $1!"
}

#freeze_project $INTERFACES
freeze_project $SYSTEM_INTERFACES
#freeze_project $FRAMEWORKS_INTERFACES
popd
