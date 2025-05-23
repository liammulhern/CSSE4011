
import { defineStore } from 'pinia'
import http from '@/utils/http'

export interface ProductOrderItem {
  product: number
  product_key: string
  quantity: number
}

export interface ProductOrderRequirement {
  requirement: number
  requirement_name: string
  unit: string
  assigned_timestamp: string
}

export interface ProductOrderTracker {
  tracker: number
  tracker_identifier: string
  assigned_timestamp: string | null
  created_timestamp: string
}

export interface ProductOrderStatus {
  status: string
  timestamp: string
  created_by: number | null
}

export interface ProductOrder {
  id: number
  order_number: string 
  supplier: number
  supplier_name: string 
  receiver: number
  receiver_name: string
  order_timestamp: string
  delivery_location: string
  items: ProductOrderItem[]
  order_requirements: ProductOrderRequirement[]
  order_trackers: ProductOrderTracker[]
  status_history: ProductOrderStatus[]
  current_status: string | null
  created_timestamp: string
  created_by: number | null
}

/** Store state */
interface ProductOrderState {
  order: ProductOrder | null
  ordersList: ProductOrder[]
  loading: boolean
  listLoading: boolean
  error: string | null
  listError: string | null

  totalOrders: number
  inTransitOrders: number
  deliveredOrders: number
  complianceAlerts: number
  totalOrdersDelta: number
  inTransitOrdersDelta: number
  deliveredOrdersDelta: number
  complianceAlertsDelta: number
}

export const useProductOrderStore = defineStore('productorder', {
  state: (): ProductOrderState => ({
    order: null,
    ordersList: [],
    loading: false,
    listLoading: false,
    error: null,
    listError: null,
    totalOrders: 0,
    inTransitOrders: 0,
    deliveredOrders: 0,
    complianceAlerts: 0,
    totalOrdersDelta: 0,
    inTransitOrdersDelta: 0,
    deliveredOrdersDelta: 0,
    complianceAlertsDelta: 0,
  }),

  actions: {
    /** Fetch a single ProductOrder by ID */
    async fetchOrder(id: string | number) {
      this.loading = true
      this.error = null
      try {
        const { data } = await http.get<ProductOrder>(
          `/api/productorders/${id}/`
        )
        this.order = data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /** Fetch list of ProductOrders */
    async fetchOrders(limit?: number, start?: string, end?: string) {
      this.listLoading = true
      this.listError   = null

      try {
        const params: Record<string,string> = {}
        if (limit)        params.page_size               = String(limit)
        if (start)        params['order_timestamp__gte'] = start
        if (end)          params['order_timestamp__lte'] = end

        const resp = await http.get<ProductOrder[]>('/api/productorders/', { params })

        this.ordersList = resp.data
      } catch (err: any) {
        this.listError = err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Fetch summary metrics: counts and deltas */
    async fetchSummary(start?: string, end?: string) {
      this.listLoading = true
      this.listError = null
      try {
        // Build query params
        const params: Record<string, string> = {}
        if (start) params.start = start
        if (end)   params.end   = end

        // Call summary endpoint
        const resp = await http.get<{
          start: string
          end: string
          total_orders: number
          total_orders_delta: number
          delivered_orders: number
          delivered_orders_delta: number
          in_transit_orders: number
          in_transit_orders_delta: number
          compliance_alerts: number
          compliance_alerts_delta: number
        }>('/api/productorders/summary/', { params })

        // Assign summary metrics
        this.totalOrders            = resp.data.total_orders
        this.totalOrdersDelta       = resp.data.total_orders_delta
        this.deliveredOrders        = resp.data.delivered_orders
        this.deliveredOrdersDelta   = resp.data.delivered_orders_delta
        this.inTransitOrders        = resp.data.in_transit_orders
        this.inTransitOrdersDelta   = resp.data.in_transit_orders_delta
        this.complianceAlerts       = resp.data.compliance_alerts
        this.complianceAlertsDelta  = resp.data.compliance_alerts_delta
      } catch (err: any) {
        this.listError = err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Clear single order state */
    clearOrder() {
      this.order = null
      this.error = null
    },

    /** Clear orders list state */
    clearOrders() {
      this.ordersList = []
      this.listError = null
    },

    /** Clear summary metrics */
    clearSummary() {
      this.totalOrders = 0
      this.inTransitOrders = 0
      this.deliveredOrders = 0
      this.complianceAlerts = 0
      this.totalOrdersDelta = 0
      this.inTransitOrdersDelta = 0
      this.deliveredOrdersDelta = 0
      this.complianceAlertsDelta = 0
    },
  },
})

