  const std::pair<*KeyValuePair, *KeyValuePair> *Traverse(Context *context_p,
                               const ValueType *value_p1,
                               const ValueType *value_p2,
                               std::pair<int, bool> *index_pair_p) {

    // For value collection it always returns nullptr
    const KeyValuePair *found_pair_p1 = nullptr;
    const KeyValuePair *found_pair_p2 = nullptr;
    const std::pair<*KeyValuePair, *KeyValuePair> *ret = nullptr;

retry_traverse:
    assert(context_p->abort_flag == false);
    #ifdef BWTREE_DEBUG
    assert(context_p->current_level == -1);
    #endif
    // This is the serialization point for reading/writing root node
    NodeID start_node_id = root_id.load();

    // This is used to identify root nodes
    // NOTE: We set current snapshot since in LoadNodeID() or read opt.
    // version the parent node snapshot will be overwritten with this child
    // node snapshot
    //
    // NOTE 2: We could not use GetLatestNodeSnashot() here since it checks
    // current_level, which is -1 at this point
    context_p->current_snapshot.node_id = INVALID_NODE_ID;

    // We need to call this even for root node since there could
    // be split delta posted on to root node
    LoadNodeID(start_node_id, context_p);

    // There could be an abort here, and we could not directly jump
    // to Init state since we would like to do some clean up or
    // statistics after aborting
    if(context_p->abort_flag == true) {
      goto abort_traverse;
    }

    bwt_printf("Successfully loading root node ID\n");

    while(1) {
      NodeID child_node_id = NavigateInnerNode(context_p);

      // Navigate could abort since it might go to another NodeID
      // if there is a split delta and the key is >= split key
      if(context_p->abort_flag == true) {
        bwt_printf("Navigate Inner Node abort. ABORT\n");

        // If NavigateInnerNode() aborts then it retrns INVALID_NODE_ID
        // as a double check
        // This is the only situation that this function returns
        // INVALID_NODE_ID
        assert(child_node_id == INVALID_NODE_ID);

        goto abort_traverse;
      }

      // This might load a leaf child
      // Also LoadNodeID() does not guarantee the node bound matches
      // seatch key. Since we could readjust using the split side link
      // during Navigate...Node()
      LoadNodeID(child_node_id, context_p);

      if(context_p->abort_flag == true) {
        bwt_printf("LoadNodeID aborted. ABORT\n");

        goto abort_traverse;
      }

      // This is the node we have just loaded
      NodeSnapshot *snapshot_p = GetLatestNodeSnapshot(context_p);

      if(snapshot_p->IsLeaf() == true) {
        bwt_printf("The next node is a leaf\n");

        break;
      }
    } //while(1)

    if(value_p1 == nullptr && value_p2 == nullptr) {
      // We are using an iterator just to get a leaf page
      assert(index_pair_p == nullptr);

      // If both are nullptr then we just navigate the sibling chain
      // to find the correct node with the correct range
      // And then the iterator will consolidate the node without actually
      // going down with a specific key
      NavigateSiblingChain(context_p);
    } else {
      // If a value is given then use this value to Traverse down leaf
      // page to find whether the value exists or not
      if (value_p1 != nullptr)
        found_pair_p1 = NavigateLeafNode(context_p, *value_p1, index_pair_p);
      if (value_p2 != nullptr)
        found_pair_p2 = NavigateLeafNode(context_p, *value_p2, index_pair_p);
      ret = std::make_pair(found_pair_p1, found_pair_p2);
    }

    if(context_p->abort_flag == true) {
      bwt_printf("NavigateLeafNode() or NavigateSiblingChain()"
                 " aborts. ABORT\n");

      goto abort_traverse;
    }

    #ifdef BWTREE_DEBUG

    bwt_printf("Found leaf node. Abort count = %d, level = %d\n",
               context_p->abort_counter,
               context_p->current_level);

    #endif

    // If there is no abort then we could safely return
    return ret;

abort_traverse:
    #ifdef BWTREE_DEBUG

    assert(context_p->current_level >= 0);

    context_p->current_level = -1;

    context_p->abort_counter++;

    #endif

    // This is used to identify root node
    context_p->current_snapshot.node_id = INVALID_NODE_ID;

    context_p->abort_flag = false;

    goto retry_traverse;

    assert(false);
    return nullptr;
  }
