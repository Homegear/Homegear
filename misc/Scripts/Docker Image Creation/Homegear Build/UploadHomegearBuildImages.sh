result=1
while [ $result -ne 0 ]; do
	docker push homegear/build
	result=$?
done
result=1
while [ $result -ne 0 ]; do
	docker push homegear/build
	result=$?
done

