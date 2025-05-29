<script setup lang="ts">
import { h, onMounted, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useProductOrderStore } from '@/stores/productorder'
import { useProductEventStore } from '@/stores/productevents'
import { useProductStore } from '@/stores/product'
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Tabs, TabsList, TabsTrigger, TabsContent } from '@/components/ui/tabs'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { parseISO, format } from 'date-fns'
import ProductEventMap from '@/components/maps/ProductEventMap.vue'

// Pinia store and router
const route = useRoute()
const router = useRouter()
const orderStore = useProductOrderStore()
const productStore = useProductStore()
const productEventStore = useProductEventStore()
const orderId = route.params.id as string

// Fetch the order on mount
onMounted(async () => {
  await orderStore.fetchOrder(orderId)
  await productStore.fetchProductsByOrder(orderId)
  await productEventStore.fetchEventsByProductOrder(orderId)
})

function formatDateTime(iso: string) {
  return new Date(iso).toLocaleString()
}

// Reactive references
const order = computed(() => orderStore.order)
const loading = computed(() => orderStore.loading)
const error = computed(() => orderStore.error)

// Computed for products list
const products = computed(() => productStore.productsList)
const prodLoading = computed(() => productStore.listLoading)
const prodError = computed(() => productStore.listError)

// Merge items (with quantity) and product details
interface MergedData {
  id: number
  product_key: string
  batch: string
  owner: number | null
  created_timestamp: string
  quantity: number
}
const mergedData = computed<MergedData[]>(() => {
  if (!order.value) return []
  return order.value.items.map(item => {
    const prod = products.value.find(p => p.id === item.product)
    return {
      id: item.product,
      product_key: prod?.product_key || item.product_key,
      product_type: prod?.product_type,
      batch: prod?.batch || '',
      owner_name: prod?.owner_name || null,
      created_timestamp: new Date(prod?.created_timestamp).toLocaleString() || '',
      quantity: item.quantity,
    }
  })
})

// Columns for merged table
const mergedColumns: ColumnDef<MergedData>[] = [
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
  { accessorKey: 'product_key', header: ({ column }) => h(DataTableHeader, { column, title: 'Product Key' }) },
  { accessorKey: 'product_type', header: ({ column }) => h(DataTableHeader, { column, title: 'Type' }) },
  { accessorKey: 'batch', header: ({ column }) => h(DataTableHeader, { column, title: 'Batch' }) },
  { accessorKey: 'owner_name', header: ({ column }) => h(DataTableHeader, { column, title: 'Owner' }) },
  { accessorKey: 'created_timestamp', header: ({ column }) => h(DataTableHeader, { column, title: 'Created At' }) },
  { accessorKey: 'quantity', header: ({ column }) => h(DataTableHeader, { column, title: 'Quantity' }) },
  {
    id: 'actions',
    header: () => ' ',
    cell: ({ row }) => h(Button, {
      variant: 'ghost', size: 'sm',
      onClick: () => router.push({ name: 'product_index', params: { id: String(row.original.id) } })
    }, () => 'View')
  }
]

// Requirements
interface ReqData { requirement_name: string; unit: string; assigned_timestamp: string }
const reqData = computed<ReqData[]>(() => order.value?.order_requirements.map(r => ({ requirement_name: r.requirement_name, unit: r.unit, assigned_timestamp: r.assigned_timestamp })) || [])
const reqColumns: ColumnDef<ReqData>[] = [
  { accessorKey: 'requirement_name', header: 'Requirement' },
  { accessorKey: 'unit', header: 'Unit' },
  { accessorKey: 'assigned_timestamp', header: 'Assigned At' },
]

// Trackers
interface TrkData { tracker_identifier: string; assigned_timestamp: string | null; created_timestamp: string }
const trkData = computed<TrkData[]>(() => order.value?.order_trackers.map(t => ({ tracker_identifier: t.tracker_identifier, assigned_timestamp: t.assigned_timestamp, created_timestamp: t.created_timestamp })) || [])
const trkColumns: ColumnDef<TrkData>[] = [
  { accessorKey: 'tracker_identifier', header: 'Tracker Key' },
  { accessorKey: 'assigned_timestamp', header: 'Attached At' },
  { accessorKey: 'created_timestamp', header: 'Assigned On' },
]

// Status History
interface StatusData { status: string; timestamp: string; created_by: number | null }
const statusData = computed<StatusData[]>(() => order.value?.status_history.map(s => ({ status: s.status, timestamp: s.timestamp, created_by: s.created_by })) || [])
const statusColumns: ColumnDef<StatusData>[] = [
  { accessorKey: 'status', header: 'Status', cell: ({ row }) => h(Badge, { variant: row.getValue() === 'delivered' ? 'success' : 'warning' }, () => String(row.getValue())) },
  { accessorKey: 'timestamp', header: 'Timestamp' },
  { accessorKey: 'created_by', header: 'By User' },
]

/**
 * Convert each ProductEvent into a LocationEvent for the map.
 *  - turn lat/lng strings + N/S, E/W into numbers
 *  - build a little HTML snippet for environment & acceleration
 */
const mapMarkers = computed(() => {
  return productEventStore.events
    .map(e => {
      const p = e.payload || {}
      const loc = p.location
      if (!loc || !loc.latitude || !loc.longitude) {
        // can’t place a marker without lat/lng
        return null
      }

      const lat = parseFloat(loc.latitude) * (loc.ns === 'S' ? -1 : 1)
      const lng = parseFloat(loc.longitude) * (loc.ew === 'W' ? -1 : 1)

      // build description only if we have data
      let desc = ''

      if (p.environment) {
        const env = p.environment
        desc += `
          <div><strong>Environment</strong><br/>
            ${env.temperature_c ? `Temp: ${env.temperature_c} °C<br/>` : ''}
            ${env.humidity_percent ? `Humidity: ${env.humidity_percent}%<br/>` : ''}
            ${env.pressure_hpa ? `Pressure: ${env.pressure_hpa} hPa<br/>` : ''}
            ${env.gas_ppm ? `Gas: ${env.gas_ppm}` : ''}
          </div>`
      }

      if (p.acceleration) {
        const a = p.acceleration
        desc += `
          <div style="margin-top:6px;"><strong>Acceleration</strong><br/>
            ${a.x_mps2 ? `X: ${a.x_mps2} m/s²<br/>` : ''}
            ${a.y_mps2 ? `Y: ${a.y_mps2} m/s²<br/>` : ''}
            ${a.z_mps2 ? `Z: ${a.z_mps2} m/s²` : ''}
          </div>`
      }

      return {
        lat,
        lng,
        title: e.event_type,
        description: desc || undefined,
        type: e.event_type,
      }
    })
    .filter((m): m is { lat: number; lng: number; title: string; description?: string; type: string } => m !== null)
})

/**
 * Center the map on the *last* event (or fallback).
 */
const mapCenter = computed<[number, number]>(() => {
  if (mapMarkers.value.length) {
    const last = mapMarkers.value[mapMarkers.value.length - 1]
    return [last.lat, last.lng]
  }
  return [39.5, -95.0]
})

</script>

<template>
  <div>
    <page-header :title="`Order #${orderId}`" />

    <div v-if="loading" class="py-4 text-center">Loading order…</div>
    <div v-else-if="error" class="py-4 text-red-500">Error: {{ error }}</div>
    <div v-else-if="!order" class="py-4 text-muted text-center">No order found.</div>
    <div v-else>
      <Card class="mb-4">
        <CardContent class="p-0">
          <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <p><strong>Supplier:</strong> {{ order.supplier_name }}</p>
              <p><strong>Receiver:</strong> {{ order.receiver_name }}</p>
              <p><strong>Placed At:</strong> {{ formatDateTime(order.order_timestamp) }}</p>
              <p><strong>Delivery Location:</strong> {{ order.delivery_location }}</p>
            </div>
            <div>
              <p><strong>Current Status:</strong>
                <Badge class="ml-3" variant="{{ order.current_status==='delivered'?'success':'warning' }}">{{
                  order.current_status }}
                </Badge>
              </p>
              <p><strong>Created On:</strong> {{ formatDateTime(order.created_timestamp) }}</p>
              <p><strong>Created By:</strong> {{ order.created_by }}</p>
            </div>
          </div>
        </CardContent>
      </Card>

      <Tabs default-value="items" class="space-y-4">
        <TabsList>
          <TabsTrigger value="items">Items</TabsTrigger>
          <TabsTrigger value="requirements">Requirements</TabsTrigger>
          <TabsTrigger value="trackers">Trackers</TabsTrigger>
          <TabsTrigger value="status">Status History</TabsTrigger>
          <TabsTrigger value="map">Map</TabsTrigger>
        </TabsList>

        <TabsContent value="items">
          <Card>
            <CardHeader>
              <CardTitle>Products</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable :columns="mergedColumns" :data="mergedData" />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="requirements">
          <Card>
            <CardHeader>
              <CardTitle>Requirements</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable :columns="reqColumns" :data="reqData" />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="trackers">
          <Card>
            <CardHeader>
              <CardTitle>Trackers</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable :columns="trkColumns" :data="trkData" />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="status">
          <Card>
            <CardHeader>
              <CardTitle>Status History</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable :columns="statusColumns" :data="statusData" />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="map" class="space-y-4">
          <Card>
            <CardHeader>
              <CardTitle>Order Locations</CardTitle>
            </CardHeader>
            <CardContent class="h-[500px]">
              <ProductEventMap :event-groups="[]" :events-no-path="mapMarkers" :center="mapCenter" :zoom="12" />
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  </div>
</template>
