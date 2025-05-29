<script setup lang="ts">
import { h, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Button } from '@/components/ui/button'
import { useProductTypeStore, type ProductType } from '@/stores/producttype'

// Define flattened row shape
interface ProductTypeRow {
  id: number
  sku: string
  name: string
  created: string
  owner: string | null
}

const router = useRouter()
const store = useProductTypeStore()

// Computed table data
const data = computed<ProductTypeRow[]>(() =>
  store.list.map((pt) => ({
    id: pt.id,
    sku: pt.product_number,
    name: pt.name,
    created: new Date(pt.created_timestamp).toLocaleString(),
    owner: pt.owner != null ? String(pt.owner) : null,
  }))
)

const loading = computed(() => store.loadingList)
const error = computed(() => store.errorList)

// Fetch on mount
onMounted(() => {
  store.fetchProductTypes()
})

// Define columns
const columns: ColumnDef<ProductTypeRow>[] = [
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
        ariaLabel: `Select ${row.original.sku}`,
        class: 'translate-y-0.5',
        enableSorting: false,
        enableHiding: false,
      }),
  },
  { accessorKey: 'sku', header: ({ column }) => h(DataTableHeader, { column, title: 'SKU' }) },
  { accessorKey: 'name', header: ({ column }) => h(DataTableHeader, { column, title: 'Name' }) },
  { accessorKey: 'created', header: ({ column }) => h(DataTableHeader, { column, title: 'Created At' }) },
  { accessorKey: 'owner', header: ({ column }) => h(DataTableHeader, { column, title: 'Owner ID' }) },
  {
    id: 'actions',
    header: () => 'Actions',
    cell: ({ row }) =>
      h(
        Button,
        {
          variant: 'ghost',
          size: 'sm',
          onClick: () => router.push({ name: 'product_type_detail', params: { id: String(row.original.id) } }),
        },
        () => 'View'
      ),
  },
]
</script>

<template>
  <div>
    <div class="mb-4 flex justify-between items-center">
      <h2 class="text-lg font-medium">Product Types</h2>
      <Button :disabled="loading" @click="store.fetchProductTypes()">
        Refresh
      </Button>
    </div>

    <div v-if="loading" class="py-4 text-center">
      Loading product types...
    </div>
    <div v-else-if="error" class="py-4 text-red-500">
      Error: {{ error }}
    </div>
    <div v-else>
      <DataTable :columns="columns" :data="data" />
    </div>
  </div>
</template>
