cmake_minimum_required(VERSION 2.8)
set( PROJECT WebGraphBuilder )

project( ${PROJECT} )
set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11" )
set(CURL_LIBRARY "-lcurl") 
find_package(CURL REQUIRED) 

include_directories(${CURL_INCLUDE_DIR})
add_executable( ${PROJECT}
				CurlWebPageDownloader.cpp
				CurlWebPageDownloader.h
				IWebPageDownloader.h
				main.cpp
				WebGraph.h
				WebGraph.cpp
				WebGraphBuilder.h
				WebGraphBuilder.cpp
				GraphmlSerialization.h
				GraphmlSerialization.cpp
				Analyze.h
				Analyze.cpp
				Common.h)

target_link_libraries( ${PROJECT} ${CURL_LIBRARIES} )
