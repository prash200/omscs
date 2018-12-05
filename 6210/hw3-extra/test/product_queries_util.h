#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <utility>

struct ProductSpec {
	const std::string vendor_id_;
	const std::string name_;
	const int count_;

	ProductSpec(const std::string vendor_id, const std::string& name, const int count)
	: vendor_id_(vendor_id), name_(name), count_(count)
	{}
};

struct ProductQueryResult {
	bool success_;
};

inline void DebugQueryPrint(const ProductSpec& spec) {
	std::cout << "DebugQueryPrint:: " \
	<< " count = " << spec.count_ \
	<< "; vendorid = " << spec.vendor_id_ \
	<< "; name = " << spec.name_ \
	<< std::endl;
}


inline bool read_product_queries(std::vector<std::vector<ProductSpec>>& queries, const std::string& filename) {

	std::ifstream myfile (filename);
	std::string line;
	std::string vendor_id;
	std::string name;
	std::string count;

	while (getline(myfile, line)) {
		std::istringstream is(line);
		std::vector<ProductSpec> query;
		while (getline(is, vendor_id, ',') && getline(is, name, ',') && getline(is, count, '\n')) {
			query.push_back(ProductSpec(vendor_id, name, stoi(count)));
			// temp
			DebugQueryPrint(ProductSpec(vendor_id, name, stoi(count)));
			//temp
			if (getline(myfile, line)) {
					is.clear();
					is.str(line);
			}
		}
		if (!query.empty()) {
			queries.push_back(query);
		}
		query.clear();
	}

	DebugQueryPrint(queries[0][0]);

	myfile.close();

	return true;
}
