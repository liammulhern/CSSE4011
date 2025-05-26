<script setup lang="ts">
import { watch, onMounted, computed, ref, h } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useProductStore } from '@/stores/product'
import http from '@/utils/http'
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Tabs, TabsList, TabsTrigger, TabsContent } from '@/components/ui/tabs'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'

// Router & Store
const route = useRoute()
const router = useRouter()
const productStore = useProductStore()

// if we were opened via QR-token URL, it’ll be in here:
const jwtToken = computed(() => route.query.token as string | undefined)

// state for QR
const qrSvg = ref<string | null>(null)
const qrLoading = ref(false)
const qrError = ref<string | null>(null)
const showQr = ref(false)

// build a reactive full-URL string (origin + path + query + hash)
const currentFullUrl = computed(() => {
  return `${window.location.origin}${route.path}`
})

async function fetchQr(id: string) {
  qrLoading.value = true
  qrError.value = null
  try {
    const res = await http.get<string>(
      `/api/products/${id}/qr-code/`,
      {
        params: { url: currentFullUrl.value, token: jwtToken.value },
        responseType: 'text',
      }
    )
    qrSvg.value = res.data
    showQr.value = true
  } catch (err: any) {
    qrError.value = err.message || String(err)
  } finally {
    qrLoading.value = false
  }
}

// turn the param into a reactive value
const productId = computed(() => route.params.id as string)

// pull all your “load this page” logic into one function
async function loadPage(id: string) {
  // if we have a token query-param, hand it off to the store
  const token = jwtToken.value;

  if (token) {
    await productStore.setJwt(token);
  }

  await productStore.fetchProduct(id);
  await productStore.fetchProductEvents(id);
  await fetchQr(id);
}

// Fetch product and its events
onMounted(() => loadPage(productId.value))

watch(productId, (newId, oldId) => {
  if (newId && newId !== oldId) {
    loadPage(newId)
  }
})

function formatDateTime(iso: string) {
  return new Date(iso).toLocaleString()
}

// Computed properties
const product = computed(() => productStore.product)
const loading = computed(() => productStore.loading)
const error = computed(() => productStore.error)

// Shape of each row in the merged components table
interface CompRow {
  id: number
  product_key: string
  batch: string
  owner_name: string | null
  created_at: string
}

// Flatten out the nested component
const compData = computed<CompRow[]>(() => {
  const list = product.value?.components ?? []
  return list.map(c => ({
    id: c.component.id,
    product_key: c.component.product_key,
    batch: c.component.batch,
    owner_name: c.component.owner_name,
    created_at: formatDateTime(c.component.created_timestamp),
  }))
})

// Columns for the merged components table
const compColumns: ColumnDef<CompRow>[] = [
  {
    accessorKey: 'product_key',
    header: ({ column }) => h(DataTableHeader, {
      column,
      title: 'Component Key'
    }),
  },
  {
    accessorKey: 'batch',
    header: ({ column }) => h(DataTableHeader, {
      column,
      title: 'Batch'
    }),
  },
  {
    accessorKey: 'owner_name',
    header: ({ column }) => h(DataTableHeader, {
      column,
      title: 'Owner'
    }),
  },
  {
    accessorKey: 'created_at',
    header: ({ column }) => h(DataTableHeader, {
      column,
      title: 'Created At'
    }),
  },
  {
    id: 'actions',
    header: () => ' ',
    cell: ({ row }) => h(Button, {
      variant: 'ghost',
      size: 'sm',
      onClick: () => {
        router.push({ name: 'product_index', params: { id: String(row.original.id) } })
      }
    }, () => 'View')
  }
]

const eventsLoading = computed(() => productStore.eventsLoading);
const eventsError = computed(() => productStore.eventsError);
const events = computed(() => productStore.events);

// Events data
interface EventData { event_type: string; timestamp: string }
const eventData = computed<EventData[]>(() => {
  const list = Array.isArray(events) ? events : []
  return list.map(e => ({
    event_type: e.event_type,
    timestamp: e.timestamp,
  }))
})

const eventColumns: ColumnDef<EventData>[] = [
  {
    accessorKey: 'event_type',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Event Type' })
  },
  {
    accessorKey: 'timestamp',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Timestamp' })
  }
]
</script>

<template>
  <div>
    <page-header :title="`Product: ${productId}`" />

    <div v-if="loading" class="py-4 text-center">Loading product…</div>
    <div v-else-if="error" class="py-4 text-red-500">Error: {{ error }}</div>
    <div v-else-if="!product" class="py-4 text-muted text-center">No product found.</div>
    <div v-else>
      <div class="flex flex-col md:flex-row items-stretch gap-4 mb-4">
        <Card class="mb-4 flex-1">
          <CardContent class="p-0">
            <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <p><strong>Key:</strong> {{ product.product_key }}</p>
                <p><strong>Type ID:</strong> {{ product.product_type }}</p>
                <p><strong>Batch:</strong> {{ product.batch }}</p>
              </div>
              <div>
                <p><strong>Owner ID:</strong> {{ product.owner }}</p>
                <p><strong>Created At:</strong> {{ formatDateTime(product.created_timestamp) }}</p>
                <p><strong>Recorded By:</strong> {{ product.recorded_by }}</p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card class="mb-4">
          <CardContent class="p-0">
            <div v-if="qrLoading" class="py-4 text-center">Loading QR code…</div>
            <div v-else-if="qrError" class="py-4 text-red-500">Error: {{ qrError }}</div>
            <div v-else-if="qrSvg" class="w-40 h-40 overflow-hidden qr-container bg-white rounded-sm" v-html="qrSvg" />
            <div v-else class="py-4 text-muted text-center">No QR code available.</div>
          </CardContent>
        </Card>
      </div>

      <Tabs default-value="composition" class="space-y-4">
        <TabsList>
          <TabsTrigger value="composition">Components</TabsTrigger>
          <TabsTrigger value="events">Events</TabsTrigger>
        </TabsList>

        <TabsContent value="composition">
          <Card>
            <CardHeader>
              <CardTitle>Component Breakdown</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable :columns="compColumns" :data="compData" :loading="loading" />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="events">
          <Card>
            <CardHeader>
              <CardTitle>Product Events</CardTitle>
            </CardHeader>
            <CardContent>
              <div v-if="eventsLoading" class="py-4 text-center">Loading events…</div>
              <div v-else-if="eventsError" class="py-4 text-red-500">Error: {{ eventsError }}</div>
              <DataTable :columns="eventColumns" :data="eventData" />
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  </div>
</template>

<style scoped>
.qr-container ::v-deep svg {
  width: 100% !important;
  height: 100% !important;
  display: block;
}
</style>
