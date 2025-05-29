<script setup lang="ts">
import { h, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router';
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { useProductOrderStore } from '@/stores/productorder'


// Define the shape of the flattened row
interface OrderData {
  id: number
  parties: string
  placedAt: string
  status: string
  transitTime: string
}

const router = useRouter();

const store = useProductOrderStore();

const props = defineProps<{
  tableName?: string;
}>();

// Compute the flat data from the store's ordersList
const data = computed<OrderData[]>(() => {
  return (store.ordersList ?? []).map(o => ({
    id: o.id,
    order_number: o.order_number,
    supplier: o.supplier_name,
    receiver: o.receiver_name,
    placedAt: new Date(o.order_timestamp).toLocaleString(),
    status: o.current_status,
    transitTime: (() => {
      const start = new Date(o.order_timestamp)
      let end = new Date()

      if (o.current_status === 'delivered') {
        const delivered = o.status_history.find(s => s.status === 'delivered')
        if (delivered) end = new Date(delivered.timestamp)
      }

      const ms = end.getTime() - start.getTime()
      const hours = Math.floor(ms / 3_600_000)
      const days = Math.floor(hours / 24)
      return days > 0 ? `${days}d ${hours % 24}h` : `${hours}h`
    })(),
  }))
})

// Proxy loading & error from the store
const loading = computed(() => store.listLoading)
const error = computed(() => store.listError)

onMounted(() => {
  store.fetchOrders();
})

const statusVariantMap: Record<string, Variant> = {
  new: 'secondary',
  on_hold: 'muted',
  delayed: 'destructive',
  shipped: 'primary',
  delivered: 'success',
}

// Define columns as before...
const columns: ColumnDef<OrderData>[] = [
  { accessorKey: 'id', header: ({ column }) => h(DataTableHeader, { column, title: '#' }) },
  { accessorKey: 'order_number', header: ({ column }) => h(DataTableHeader, { column, title: 'Order Code' }) },
  { accessorKey: 'supplier', header: ({ column }) => h(DataTableHeader, { column, title: 'Supplier' }) },
  { accessorKey: 'receiver', header: ({ column }) => h(DataTableHeader, { column, title: 'Receiver' }) },
  { accessorKey: 'placedAt', header: ({ column }) => h(DataTableHeader, { column, title: 'Placed At' }) },
  {
    accessorKey: 'status',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Status' }),
    cell: ({ row }) => {
      const status = String(row.getValue('status'))  // e.g. 'new', 'delivered', etc.
      const variant = statusVariantMap[status] ?? 'secondary'
      // Capitalize or humanize the label if you like:
      const label = status.replace('_', ' ').replace(/\b\w/g, c => c.toUpperCase())

      return h(
        Badge,
        { variant },
        () => label
      )
    },
  },
  {
    accessorKey: 'transitTime',
    header: ({ column }) => h(DataTableHeader, {
      column,
      title: 'Transit Time'
    })
  },
  {
    id: 'actions',
    header: () => 'Actions',
    cell: ({ row }) => h(Button, {
      variant: 'ghost',
      onClick: () => router.push({ name: 'product_order_index', params: { id: String(row.original.id) } })
    }, () => 'View')
  }
]
</script>

<template>
  <div>
    <div class="mb-4 flex justify-between items-center">
      <h2 class="text-lg font-medium"> {{ props.tableName ?? 'Product Orders' }}</h2>
      <Button :disabled="loading" @click="store.fetchOrders()">
        <Icon name="RefreshCw" class="pr-1" />
        Refresh
      </Button>
    </div>
    <div v-if="loading" class="py-4 text-center">Loading orders…</div>
    <div v-else-if="error" class="py-4 text-red-500">Error: {{ error }}</div>
    <div v-else>
      <DataTable :columns="columns" :data="data" />
    </div>
  </div>
</template>
