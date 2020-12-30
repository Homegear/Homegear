#!/bin/bash

json_escape()
{
  printf '%s' "$1" | python -c 'import json,sys; print(json.dumps(sys.stdin.read()))'
}

get_output()
{
  output="{\"name\":\"Node-BLUE Community Catalog\",\"modules\":["

  for file in node-blue-node-*_${1}.deb; do
    package=$(dpkg --info $file | grep "^ Package:" | cut -d ":" -f 2- | xargs)
    package="${package/node-blue-node-/}"
    package=$(json_escape "$package")
    version=$(dpkg --info $file | grep "^ Version:" | cut -d ":" -f 2- | xargs)
    version=$(json_escape "$version")
    homepage=$(dpkg --info $file | grep "^ Homepage:" | cut -d ":" -f 2- | xargs)
    homepage=$(json_escape "$homepage")
    description=$(dpkg --info $file | grep "^ Description:" | cut -d ":" -f 2- | xargs)
    description=$(json_escape "$description")
    keywords=$(dpkg --info $file | grep "^  Keywords:" | cut -d ":" -f 2- | xargs)
    keywordsJson="["
    keywordsArray=(${keywords//,/ })
    for keyword in "${keywordsArray[@]}"; do
      keyword=$(json_escape "$keyword")
      keywordsJson="${keywordsJson}${keyword},"
    done
    if [ ${#keywordsJson} -eq 1 ]; then
      keywordsJson="${keywordsJson}]"
    else
      keywordsJson="${keywordsJson::-1}]"
    fi

    line=$(dpkg --info $file | grep -n "^ Description:" | cut -d ":" -f 1)
    line=$((line + 1))
    longDescription=$(dpkg --info $file | tail -n +$line)
    longDescription=$(json_escape "$longDescription")

    json=$(cat << EOF
  {"id":$package,"version":$version,"url":$homepage,"description":$description,"keywords":$keywordsJson,"updated_at":"$(TZ=UTC date -Iminutes)"}
EOF
    )
    output="${output}${json},"
  done

  output="${output::-1}]}"
  echo $output
}

if test -z $1; then
    echo "Please specify a repository path."
    exit 1;
fi

cd $1
get_output all > catalog_all.json
get_output amd64 > catalog_amd64.json
get_output i386 > catalog_i386.json
get_output armhf > catalog_armhf.json
get_output arm64 > catalog_arm64.json
