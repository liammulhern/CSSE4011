<script setup lang="ts">
import { h, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Button } from '@/components/ui/button'
import { useProductStore } from '@/stores/product'

// Define the shape of each flattened product row
interface ProductRow {
  id: number
  key: string
  batch: string
  owner: string | null
  created: string
  components: number
}

const router = useRouter()
const store = useProductStore()

// Compute flattened data from productsList
const data = computed<ProductRow[]>(() =>
  store.productsList.map((p) => ({
    id: p.id,
    key: p.product_key,
    batch: p.batch,
    owner: p.owner_name,
    created: new Date(p.created_timestamp).toLocaleString(),
    components: p.components.length,
  }))
)

const loading = computed(() => store.listLoading)
const error = computed(() => store.listError)

// Fetch the list on mount
onMounted(() => {
  store.fetchProducts()
})

// Table columns definition
const columns: ColumnDef<ProductRow>[] = [
  {
    accessorKey: 'id',
    header: ({ table }) =>
      h(Checkbox, {
        checked: table.getIsAllPageRowsSelected(),
        'onUpdate:checked': (val: boolean) => table.toggleAllPageRowsSelected(val),
        ariaLabel: 'Select All',
        class: 'translate-y-0.5',
      }),
    cell: ({ row }) =>
      h(Checkbox, {
        checked: row.getIsSelected(),
        'onUpdate:checked': (val: boolean) => row.toggleSelected(val),
        ariaLabel: 'Select row',
        class: 'translate-y-0.5',
        enableSorting: false,
        enableHiding: false,
      }),
  },
  {
    accessorKey: 'key',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Product Key' }),
  },
  {
    accessorKey: 'batch',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Batch' }),
  },
  {
    accessorKey: 'owner',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Owner' }),
  },
  {
    accessorKey: 'created',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Created At' }),
  },
  {
    accessorKey: 'components',
    header: ({ column }) => h(DataTableHeader, { column, title: '# Components' }),
    cell: ({ getValue }) => String(getValue()),
  },
  {
    id: 'actions',
    header: () => 'Actions',
    cell: ({ row }) =>
      h(
        Button,
        {
          variant: 'ghost',
          size: 'sm',
          onClick: () => router.push({ name: 'product_index', params: { id: String(row.original.id) } }),
        },
        () => 'View'
      ),
  },
]
</script>

<template>
  <div>
    <div class="mb-4 flex justify-between items-center">
      <h2 class="text-lg font-medium">Products</h2>
      <Button :disabled="loading" @click="store.fetchProducts()">
        <Icon name="RefreshCw" class="pr-1" />
        Refresh
      </Button>
    </div>

    <div v-if="loading" class="py-4 text-center">
      Loading products…
    </div>
    <div v-else-if="error" class="py-4 text-red-500">
      Error: {{ error }}
    </div>
    <div v-else>
      <DataTable :columns="columns" :data="data" />
    </div>
  </div>
</template>
