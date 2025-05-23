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
  supplier: number
  receiver: number
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
  loading: boolean
  error: string | null
}

export const useProductOrderStore = defineStore('productOrder', {
  state: (): ProductOrderState => ({
    order: null,
    loading: false,
    error: null,
  }),

  actions: {
    /**
     * Fetch a single ProductOrder by ID
     */
    async fetchOrder(id: string | number) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<ProductOrder>(
          `/api/product-orders/${id}/`
        )
        this.order = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },

    /**
     * Clear out the current order
     */
    clearOrder() {
      this.order = null
      this.error = null
    },
  },
})

