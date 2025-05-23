
import { defineStore } from 'pinia'
import http from '@/utils/http'

/**
 * The shape of a “nested” product as returned inside components[]
 */
export interface NestedProduct {
  id: number
  product_key: string
  product_type: number | null
  batch: string
  owner: number | null
  created_timestamp: string
}

/** Now each component entry has the full NestedProduct plus a quantity */
export interface ProductComponent {
  component: NestedProduct
  quantity: number
}

export interface Product {
  id: number
  product_key: string
  product_type: number | null
  batch: string
  owner: number | null
  owner_name: string | null
  created_timestamp: string
  components: ProductComponent[]
  recorded_by: number | null
}

interface ProductState {
  // Single-product detail
  product: Product | null
  loading: boolean
  error: string | null

  // List of products for an order
  productsList: Product[]
  listLoading: boolean
  listError: string | null
}

export const useProductStore = defineStore('product', {
  state: (): ProductState => ({
    product: null,
    loading: false,
    error: null,

    productsList: [],
    listLoading: false,
    listError: null,
  }),

  actions: {
    /** Fetch a single product by ID */
    async fetchProduct(id: string | number) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<Product>(`/api/products/${id}/`)
        this.product = data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /** Fetch all products linked to a given ProductOrder */
    async fetchProductsByOrder(orderId: string | number) {
      this.listLoading = true
      this.listError = null

      try {
        const { data } = await http.get<Product[]>(
          `/api/products/order/${orderId}/`
        )
        this.productsList = data
      } catch (err: any) {
        this.listError = err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Clear single-product state */
    clearProduct() {
      this.product = null
      this.error = null
    },

    /** Clear products list state */
    clearProductsList() {
      this.productsList = []
      this.listError = null
    },
  },
})

