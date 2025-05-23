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
  actionNeeded: string
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
    status: o.current_status ?? 'N/A',
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
    actionNeeded: o.current_status !== 'delivered' ? 'Yes' : 'No',
  }))
})

// Proxy loading & error from the store
const loading = computed(() => store.listLoading)
const error = computed(() => store.listError)

onMounted(() => {
  store.fetchOrders();
})

// Define columns as before...
const columns: ColumnDef<OrderData>[] = [
  {
    accessorKey: 'id',
    header: ({ table }) => h(Checkbox, {
      checked: table.getIsAllPageRowsSelected(),
      'onUpdate:checked': val => table.toggleAllPageRowsSelected(!!val),
      ariaLabel: 'Select All',
      class: 'translate-y-0.5',
    }),
    cell: ({ row }) => h(Checkbox, {
      checked: row.getIsSelected(),
      'onUpdate:checked': val => row.toggleSelected(!!val),
      ariaLabel: 'Select row',
      class: 'translate-y-0.5',
      enableSorting: false,
      enableHiding: false,
    }),
  },
  { accessorKey: 'order_number', header: ({ column }) => h(DataTableHeader, { column, title: 'Order #' }) },
  { accessorKey: 'supplier', header: ({ column }) => h(DataTableHeader, { column, title: 'Supplier' }) },
  { accessorKey: 'receiver', header: ({ column }) => h(DataTableHeader, { column, title: 'Receiver' }) },
  { accessorKey: 'placedAt', header: ({ column }) => h(DataTableHeader, { column, title: 'Placed At' }) },
  {
    accessorKey: 'status',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Status' }),
    cell: ({ row }) => h(Badge, {
      variant: row.getValue() === 'delivered' ? 'success' : 'warning'
    }, () => String(row.getValue()))
  },
  { accessorKey: 'transitTime', header: 'Transit Time' },
  {
    accessorKey: 'actionNeeded',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Action Needed?' }),
    cell: ({ getValue }) => h('span', {
      class: getValue() === 'Yes' ? 'text-red-600 font-semibold' : 'text-green-600'
    }, getValue())
  },
  {
    id: 'actions',
    header: () => ' ',
    cell: ({ row }) => h(Button, {
      variant: 'ghost',
      onClick: () => router.push({ name: 'product_order_id', params: { id: String(row.original.id) } })
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
