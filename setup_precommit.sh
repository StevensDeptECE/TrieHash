if ! pip --version &> /dev/null
then
	if ! python3 --version &> /dev/null
	then
		echo "Please install python 3: sudo apt update && sudo apt install python3.6"
	else
		echo "Please install pip: sudo apt update && sudo apt install python3-pip"
	fi
else
	# Pip3 was found
	pip3 install pre-commit
	pre-commit install
	pre-commit run --all-files
fi

echo "If anything went wrong, please run the following: sudo apt update && sudo apt install cppcheck clang-format"
