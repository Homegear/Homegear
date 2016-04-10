#!/bin/bash
result=1
while [ $result -ne 0 ]; do
	docker push homegear/phpbuild
	result=$?
	sleep 5
done
result=1
while [ $result -ne 0 ]; do
	docker push homegear/phpbuild
	result=$?
	sleep 5
done
