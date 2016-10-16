# Contributing
## 1. Fork the repository
## 2. Clone the forked repository
## 3. Create a new branch
```bash
# Create a new branch
git checkout -b my_new_branch
```

## 4. Build the entire library
```bash
# Install the development dependencies (GoogleTest)
./install_deps.sh
# Build the entire library as well as the tests
mkdir build
cd build
cmake .. -DBUILD_TESTS=true -DBUILD_EXAMPLES=true -DLOGGING_ENABLED=1
make
# Run tests and examples
./bin/cpp_redis_tests
./bin/redis_subscriber
./bin/redis_client
```

## 5. Code your changes
Develop your new features or bugfix.

Please:
* follow the same coding style and convention used in the existing code
* the library, examples and tests are all still compiling
* ensure that all the tests are passing on your computer at every step of the development
* add some tests if you are developing new features

You also need to use the formatting tool so that your code has the same coding style as the existing code:

```bash
# Use the formatting tool
./clang-format
```

## 7. Commit your changes
```bash
git add .
git commit -m 'some description of the changes'
```
You can do as many commits as you want: we will squash them into a single commit.

## 8. Before the Pull Request
Before submitting the pull request, ensure that:
* your feature works as expected and is tested
* all tests pass on both your computer and the [Travis](travis-ci.org/Cylix/cpp_redis)
* you have used the formatting tool

## 9. Submit your Pull Request on Github
