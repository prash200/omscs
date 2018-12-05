# CS 6210: Distributed Service using grpc - Project 3 Extra Credit
This is an extra credit project based on project 3.
## Overview.
In this project, the store acts as a mediator and makes a transaction between the clients and the vendors. The vendors will have items in one of three states (available, reserved and sold). The client sends a request to the store with the vendors, product names and count of items needed. The store tries to buy these items from the vendors replies back whether the requested transaction was successful or not.

## Dependencies
Same as project 3.

## Vendors
Vendors will be associated with ip_address and a config file.

`vendor_ipaddresses.txt` or the file given to run_vendors.cc will have format:
```
    vendor_ip_addr1;config_file1
    vendor_ip_addr2;config_file2
```
Config file format :
```
     product_name1;count1
     product_name2;count2
     product_name3;count3
```

Each vendor will read its config file and initialize the items it has at the beginning. Then vendor provides an interface to reserve, release, sell products. All three `return bool` which if `true` means the operation was successful else not.
1. Reserve - Reserve `count` items of `product_name`. This will move items from available state to reserved.
2. Release - Release `count` items of `product_name`. This will move items from reserved back to available.
3. Sell - Sell `count` items of `product_name`. This will move items from reserved state to sold state.

Each of the above 3 apis take `TransactionQuery` defined in `vendor.proto` as param and return a `TransactionReply`.

A successful sale would first `Reserve` the items and then call `Sell` on them.
After reserving, if you wish to cancel it you can call `Release` and the items will go back to the available pool.

Look at `unit_test_vendor.cc` for example usage of above vendor apis.

## Client
In this project, the client is trying to make a transaction (or purchase) that works in an atomic way -- that is, either all products in the query are purchased on none of them are purchased. Queries are stored in `product_queries_util.txt`, which uses blank line to separate queries. Each query has **a list of** products and its corresponding quantities, as well as desired vendor to buy from.
```
    localhost:50051,book1,3       --> Query1
    localhost:50054,trimmer1,5    --> Query1
                                  --> new line separates two queries
    localhost:50051,book1,3       --> Query2
    localhost:50051,trimmer1,5    --> Query2
```
Each time, the client will make a query (containing multiple products) to the store, and expects store to return true or false, representing whether the purchase succeeds or not.

When doing the testing, we may use multiple clients to send queries at the same time. That's why we need an asynchronous server.

The interface of the client and the store is specified in store.proto, which specifies that each purchase query contains a list of product items. And the response expected is simply whether or not the purchase succeeds.

## Store
Store takes client requests and tries to buy the items from all vendors mentioned in the request. This transaction either succeeds completely or fails completely i.e. a client request cannot be half fulfilled. To make it more clear, consider an example :
Suppose the client sends the following request :
```
     vendor1-product1-10 --> q1
     vendor2-product2-10 --> q2
```
The above single request from client requires the store to go talk to two vendors.
Now the store will do the following steps :
1. Reserve `product1-10` from vendor1.
2. Reserve `product2-10` from vendor2.
3. If both 1 and 2 succeed, call sell with `product1-10` for vendor1 and `product2-10` for vendor2. Reply to client saying transaction was successful.
4. If either 1 or 2 fail, this means we cannot complete some part of client request.
   1. For the parts of the request that had successfully reserved, you should call release on them to release those items back to the available pool.
   2. Say reserve of q1 succeeded and q2 failed, then you would call `release` on q1. This would make `product1-10` on vendor1 go back to available pool.
   3. Then you should reply back to the client saying the transaction failed.

## Run
1. run store
2. run run_tests, passing store's ip.

## Instructions
1. Implement store based on the logic described above.
2. You're not allowed to modify the store.proto and vendor.proto files.
3. You're not allowed to modify the vendor implementation.
4. A sample client has been given to you.
5. For grading, your store will be tested against more clients and vendors than given in example. You are encouraged to write additional clients and new config files for vendors and test your store can correctly handle various scenarios.

