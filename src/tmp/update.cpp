bool Update(const KeyType &key, const ValueType &oldValue, const ValueType &newValue) {
  bwt_printf("Update called\n");

  #ifdef BWTREE_DEBUG
  update_op_count.fetch_add(1);
  #endif

  EpochNode *epoch_node_p = epoch_manager.JoinEpoch();

  /* update will be performed iff old value is exist and new value is not.
   * YOU MUST use previous one to be percise, but in order to make test case easier
   * we DO NOT check for the newValue's existence
   */
    while(1) {
      Context context{key};
      std::pair<int, bool> index_pair;

      // Navigate leaf nodes to check whether the key-oldValue & key-newValue
      // pair exists
      const std::pair<*KeyValuePair, *KeyValuePair> *item_p = Traverse(&context, &oldValue, &newValue, &index_pair);

      /* There is 3 reason for update() to fail
       * 1. Traverse() aborted, returning nullptr
       * 2. <key,oldValue> does not exist
       * 3. <key,newValue> already exist
       */
      if(item_p == nullptr || item_p.first == nullptr || item_p.second != nullptr) {
        #ifdef BWTREE_DEBUG
        update_abort_count.fetch_add(1);
        #endif
        epoch_manager.LeaveEpoch(epoch_node_p);

        return false;
      }

      NodeSnapshot *snapshot_p = GetLatestNodeSnapshot(&context);

      // We will CAS on top of this
      const BaseNode *node_p = snapshot_p->node_p;
      NodeID node_id = snapshot_p->node_id;

      const LeafDeleteNode *delete_node_p = \
        LeafInlineAllocateOfType(LeafDeleteNode,
                                 node_p,
                                 key,
                                 oldValue,
                                 node_p,
                                 index_pair);

     const LeafInsertNode *insert_node_p = \
       LeafInlineAllocateOfType(LeafInsertNode,
                                node_p,
                                key,
                                newValue,
                                delete_node_p,
                                index_pair);// delete_node_p on 5th argv.

      bool ret = InstallNodeToReplace(node_id,
                                      insert_node_p,
                                      node_p);// install insert_node_p
      if(ret == true) {
        break;
      } else {
        delete_node_p->~LeafDeleteNode();
      }
    }// delete & insert key-oldValue pair done
  epoch_manager.LeaveEpoch(epoch_node_p);

  return true;
}
