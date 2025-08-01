<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import * as z from 'zod'
import http from '@/utils/http'
import { toast } from '@/components/ui/toast'
import { Button } from '@/components/ui/button'
import {
  FormField,
  FormItem,
  FormLabel,
  FormControl,
  FormMessage,
} from '@/components/ui/form'
import { Input } from '@/components/ui/input'
import {
  Dialog,
  DialogTrigger,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogDescription,
  DialogFooter,
  DialogClose,
} from '@/components/ui/dialog'

const router = useRouter()
const showDialog = ref(false)

// Validation schema for IoT Hub message
const formSchema = toTypedSchema(
  z.object({
    deviceID: z.string().min(1, 'Device ID is required'),
    messageType: z.string().min(1, 'Message Type is required'),
    message: z.string().min(1, 'Message payload is required'),
  })
)

// Setup Vee-Validate form
const { handleSubmit, values, resetForm } = useForm({ validationSchema: formSchema })
const loading = ref(false)

// Submit handler
const onSubmit = handleSubmit(async (vals) => {
  loading.value = true
  try {
    await http.post('/api/iot-hub/send/', {
      deviceID: vals.deviceID,
      messageType: vals.messageType,
      message: vals.message,
    })
    toast({ title: 'Message Sent', description: 'Your message was queued successfully.' })
    resetForm()
    showDialog.value = false
    // Optionally navigate somewhere
    // router.push({ name: 'dashboard' })
  } catch (err: any) {
    toast({
      title: 'Error',
      description: err.response?.data?.detail || err.message,
      variant: 'destructive',
    })
  } finally {
    loading.value = false
  }
})
</script>

<template>
  <Dialog v-model:open="showDialog">
    <DialogTrigger asChild>
      <Button variant="outline">
        Send IoT Hub Message
      </Button>
    </DialogTrigger>

    <DialogContent class="sm:max-w-lg">
      <DialogHeader>
        <DialogTitle>Send IoT Hub Message</DialogTitle>
        <DialogDescription>
          Fill in the details below to queue a message to your device.
        </DialogDescription>
      </DialogHeader>

      <form @submit.prevent="onSubmit" class="space-y-4">
        <FormField name="deviceID" v-slot="{ field }">
          <FormItem>
            <FormLabel>Device ID</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="e.g. pathledger-gateway-uart-0" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="messageType" v-slot="{ field }">
          <FormItem>
            <FormLabel>Message Type</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="e.g. deviceIDUpdate" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="message" v-slot="{ field }">
          <FormItem>
            <FormLabel>Message Payload</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="Enter payload (string or number)" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <DialogFooter class="flex justify-end space-x-2">
          <DialogClose asChild>
            <Button variant="secondary" type="button">
              Cancel
            </Button>
          </DialogClose>
          <Button :loading="loading" type="submit">
            Send
          </Button>
        </DialogFooter>
      </form>
    </DialogContent>
  </Dialog>
</template>

