#!/bin/bash

git_push()
{
  URL=$(git config remote.origin.url | sed "s/https:\/\//https:\/\/$(cat ~/.git-token)@/g")
  git push $URL
}
