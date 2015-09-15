result=1
while [ $result -ne 0 ]; do
	docker push homegear/phpbuild
	result=$?
done
